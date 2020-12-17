package org.jpype.python;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Stream;
import org.jpype.asm.ClassReader;
import org.jpype.asm.ClassVisitor;
import org.jpype.asm.ClassWriter;
import org.jpype.asm.FieldVisitor;
import org.jpype.asm.MethodVisitor;
import static org.jpype.asm.Opcodes.*;
import org.jpype.asm.Type;
import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyBaseExtension;
import org.jpype.python.internal.PyBaseStatic;
import python.lang.PyEngine;

/**
 * Builder to create dynamic classes for use with Python.
 *
 * This uses the ASM library to dynamically compile classes. They must be loaded
 * into a custom class loader to be activated.
 */
class PyTypeBuilder
{

  String invoker = Type.getInternalName(PyInvoker.class);

  public byte[] newClass(String name, Class concrete, Class... interfaces)
  {

    if (concrete != null)
    {
      if (!PyBaseObject.class.isAssignableFrom(concrete)
              && concrete.getSuperclass().equals(Object.class))
        throw new RuntimeException(concrete + " is missing base.");
    } else
    {
      concrete = PyBaseObject.class;
      if (interfaces.length == 1)
      {
        PyTypeInfo info = (PyTypeInfo) interfaces[0].getAnnotation(PyTypeInfo.class);
        if (info == null)
          concrete = Object.class;
      }
    }

//    System.out.println("Build class " + name);
//    if (concrete != null)
//      System.out.println("    concrete " + concrete);
//    for (Class b : interfaces)
//      System.out.println("    " + b);

    // Find a unique set of methods
    HashSet<String> set = new HashSet<>();
    ArrayList<Long> entries = new ArrayList<>();
    ArrayList<Method> methods = new ArrayList<>();
    Class internal = null;

    try
    {
      for (Class cls : interfaces)
      {
        PyTypeInfo info = (PyTypeInfo) cls.getAnnotation(PyTypeInfo.class);
        collectMethods(cls, set, entries, methods);
        if (info != null && info.internal() != PyTypeInfo.class)
        {
          collectMethods(info.internal(), set, entries, methods);
          if (internal == null)
            internal = info.internal();
        }
      }
    } catch (Throwable th)
    {
      th.printStackTrace();
    }

    byte[] out = generateClass(name, concrete, internal, interfaces, methods, entries);

    // Write the class to disk for inspection with javap -c
    try ( OutputStream os = Files.newOutputStream(Paths.get(name + ".class")))
    {
      os.write(out);
    } catch (IOException ex)
    {
      throw new RuntimeException(ex);
    }
    return out;
  }

  /**
   * Search for methods to be implemented.
   */
  private void collectMethods(Class cls, Set<String> set, List<Long> entries, List<Method> methods)
  {
    for (Method method : cls.getMethods())
    {
      try
      {
        PyMethodInfo info = method.getAnnotation(PyMethodInfo.class);
        if (info == null)
          continue;
        StringBuilder sb = new StringBuilder();
        sb.append(method.getName()).append(":");
        sb.append(Type.getMethodDescriptor(method));
        String sig = sb.toString();
        if (set.contains(sig))
          continue;
        set.add(sig);
        entries.add(getMethod(info.name()));
        methods.add(method);
      } catch (NoSuchMethodException ex)
      {
        throw new RuntimeException(ex);
      }
    }
  }

  static public long getMethod(String signature) throws NoSuchMethodException
  {
    Long l = PyEngine.getInstance().getSymbol(signature);
    if (l == null)
    {
      throw new NoSuchMethodException(signature);
    }
    return l;
  }

//<editor-fold desc="asm" defaultstate="collapsed">
  /**
   * Construct a class based on the requested interfaces.
   *
   * Declares the class, implements any methods that have PyMethodInfo
   * annotation, copies in private implementation methods, and copies in base
   * object methods.
   *
   * @param name
   * @param concrete
   * @param internal
   * @param interfaces
   * @param methods
   * @param entries
   * @return
   */
  private byte[] generateClass(String name,
          Class concrete,
          Class internal,
          Class[] interfaces,
          List<Method> methods,
          List<Long> entries)
  {
    String[] interfaceNames = Stream.of(interfaces).map(p -> Type.getInternalName(p)).toArray(String[]::new);
    ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_MAXS);
    ClassSpecification target = new ClassSpecification(cw, name, concrete, internal, interfaces);
    cw.visit(V1_8, ACC_PUBLIC | ACC_SUPER | ACC_FINAL, name, null, Type.getInternalName(concrete), interfaceNames);

    // Add methods
    int index = 0;
    for (Method method : methods)
    {
      PyMethodInfo info = method.getAnnotation(PyMethodInfo.class);
      generateMethod(target, method, info, entries.get(index));
      index++;
    }
    if (internal != null)
    {
      copyClass(target, internal);
    }
    if (concrete != Object.class)
      copyClass(target, PyBaseExtension.class);
    else
      copyClass(target, PyBaseStatic.class);
    cw.visitEnd();
    return cw.toByteArray();
  }

  /**
   * Implements a method.
   *
   * All methods marked with PyMethodInfo will be implemented as native. As we
   * can't create C function stubs on the fly easily we will proxy through
   * PyInvoker. These methods are simple. We look up the Java context, get the
   * entry point from the method cache, copy the arguments onto the call stack,
   * call invoker, and then match the return against the requested return type.
   *
   * @param target
   * @param method
   * @param info
   * @param entry
   */
  private void generateMethod(ClassSpecification target, Method method,
          PyMethodInfo info, Long entry)
  {
    ClassWriter cw = target.cw;
    PyInvocation invocation = info.invoke();
    Method imethod = invocation.getMethod();

    // Check if this is already implemented
    String name = method.getName();
    String descriptor = Type.getMethodDescriptor(method);
    if (target.checkCache(name + descriptor))
      return;

    MethodVisitor mv = cw.visitMethod(ACC_PUBLIC, name,
            descriptor, null, null);
    mv.visitCode();

    // First thing we need the entry point
    mv.visitLdcInsn(entry);

    // If there are special flags they go next
    mv.visitLdcInsn(info.flags());

    // Pass the parameters
    Class[] params = imethod.getParameterTypes();
    int pcount = 0;

    // If we are not a method, then skip this.
    if (!info.method())
      pcount++;
    int lparams = params.length;
    if (info.op() != -1)
      lparams--;

    // The 2 in the next line is how many invoker parameters to skip
    for (int i = 2; i < lparams; ++i)
    {
      Class param = params[i];
      if (param == Integer.TYPE)
        mv.visitVarInsn(ILOAD, pcount++);
      else if (param == Long.TYPE)
      { // Long counts as 2 slots
        mv.visitVarInsn(LLOAD, pcount++);
        pcount++;
      } else if (param == Float.TYPE)
        mv.visitVarInsn(FLOAD, pcount++);
      else if (param == Double.TYPE)
      { // Double counts as 2 slots
        mv.visitVarInsn(DLOAD, pcount++);
        pcount++;
      } else
        mv.visitVarInsn(ALOAD, pcount++);
    }

    // Add a fixed opcode if applicable. (for richcompare)
    if (info.op() != -1)
      mv.visitLdcInsn(info.op());

    // Call the invoke method
    mv.visitMethodInsn(INVOKESTATIC, invoker, imethod.getName(),
            Type.getMethodDescriptor(imethod), false);

    // Handle the return
    Class<?> retType = method.getReturnType();
    if (retType == Void.TYPE)
      mv.visitInsn(RETURN);
    else if (retType == Boolean.TYPE)
      mv.visitInsn(IRETURN);
    else if (retType == Integer.TYPE)
    {
      if (imethod.getReturnType() == Long.TYPE)
        mv.visitInsn(L2I);
      mv.visitInsn(IRETURN);
    } else if (retType == Long.TYPE)
      mv.visitInsn(LRETURN);
    else if (retType == Float.TYPE)
      mv.visitInsn(FRETURN);
    else if (retType == Double.TYPE)
      mv.visitInsn(DRETURN);
    else if (retType == Object.class)
      mv.visitInsn(ARETURN);
    else
    {
      // Everything else needs a cast
      mv.visitTypeInsn(CHECKCAST, Type.getInternalName(retType));
      mv.visitInsn(ARETURN);

    }
    mv.visitMaxs(0, 0);
    mv.visitEnd();
  }
//</editor-fold>
//<editor-fold desc="inner" defaultstate="collapsed">

  /**
   * Structure to hold all the variables for a new class wrapper during
   * construction.
   */
  private static class ClassSpecification
  {

    private final ClassWriter cw;
    private final String name;
    private final HashSet<String> implemented_ = new HashSet<>();
    Class concrete;
    Class internal;
    Class[] interfaces;

    ClassSpecification(ClassWriter cw, String targetName, Class concrete,
            Class internal,
            Class[] interfaces)
    {
      this.cw = cw;
      this.name = targetName;
      this.concrete = concrete;
      this.interfaces = interfaces;
      this.internal = internal;
    }

    /**
     * Cache to see what has been implemented.
     *
     * Classes can only implement fields and methods once. This cache is ensures
     * we skip any replication. If two classes have a conflict it will be
     * determined when the class is loaded.
     *
     * @param key
     * @return
     */
    private boolean checkCache(String key)
    {
      if (implemented_.contains(key))
        return true;
      implemented_.add(key);
      return false;
    }
  }
//</editor-fold>
//<editor-fold desc="copy" defaultstate="collapsed">

  void copyClass(ClassSpecification target, Class source)
  {
    try ( InputStream is = source.getClassLoader().getResourceAsStream(Type.getInternalName(source) + ".class"))
    {
      ClassReader cr = new ClassReader(is);
      ClassCopy cc = new ClassCopy(target, source, source.getSuperclass());
      // Delegate to the copy method.
      cr.accept(cc, 0);
    } catch (IOException ex)
    {
      throw new RuntimeException("Unable to find " + Type.getInternalName(source), ex);
    }
  }

  static class ClassCopy extends ClassVisitor
  {

    private String sourceName;
    private final ClassSpecification target;
    private final Class sourceClass;
    private final Class superClass;

    public ClassCopy(ClassSpecification target, Class sourceClass, Class superClass)
    {
      super(ASM8);
      this.target = target;
      this.sourceClass = sourceClass;
      this.superClass = superClass;
    }

    @Override
    public void visit(int version, int access, String name, String signature, String superName, String[] interfaces)
    {
      this.sourceName = name;
    }

    @Override
    public FieldVisitor visitField(int access, String name, String descriptor, String signature, Object value)
    {
      if (target.checkCache(name))
        return null;
      target.cw.visitField(access, name, descriptor, signature, value);
      return null;
    }

    @Override
    public MethodVisitor visitMethod(int access, String name, String descriptor, String signature, String[] exceptions)
    {
      // Don't copy abstract
      if ((access & ACC_ABSTRACT) == ACC_ABSTRACT)
        return null;

      if (target.checkCache(name + descriptor))
        return null;

      String superName = null;
      if (superClass != null)
        superName = Type.getInternalName(superClass);

      return new MethodCopy(target.cw.visitMethod(access, name, descriptor, signature, exceptions),
              name, target, sourceName, superName);
    }
  }

  static class MethodCopy extends MethodVisitor
  {

    private final ClassSpecification target;
    private final String sourceName;
    private final String methodName;
    private final String superName;

    private MethodCopy(MethodVisitor visitMethod, String methodName, ClassSpecification target,
            String sourceName, String superName)
    {
      super(ASM8, visitMethod);
      this.methodName = methodName;
      this.target = target;
      this.sourceName = sourceName;
      this.superName = superName;
    }

    @Override
    public void visitFieldInsn(int opcode, String owner, String name, String descriptor)
    {
      if (owner.equals(sourceName))
        mv.visitFieldInsn(opcode, target.name, name, descriptor);
      else
        mv.visitFieldInsn(opcode, owner, name, descriptor);
    }

    @Override
    public void visitMethodInsn(int opcode, String owner, String name, String descriptor, boolean isInterface)
    {
      if (methodName.equals("<init>") && opcode == INVOKESPECIAL
              && target.concrete != null && owner.equals(superName))
        owner = Type.getInternalName(target.concrete);
      if (owner.equals(sourceName))
        mv.visitMethodInsn(opcode, target.name, name, descriptor, isInterface);
      else
        mv.visitMethodInsn(opcode, owner, name, descriptor, isInterface);
    }

    @Override
    public void visitTypeInsn(int opcode, String type)
    {
      if (type.equals(sourceName))
        mv.visitTypeInsn(opcode, target.name);
      else
        mv.visitTypeInsn(opcode, type);
    }
  }
//</editor-fold>
}
