// --- file: org/jpype/proxy/JPypeProxyType.java ---
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import org.jpype.annotation.Builtin;
import org.jpype.internal.NativeContext;
import org.jpype.manager.StringManager;
import org.jpype.manager.TypeManager;
import org.jpype.annotation.Bypass;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import python.lang.PyBuiltIn;

public final class ProxyType
{

  final PyBuiltIn builtin;
  final NativeContext context;
  final StringManager stringManager;
  final TypeManager typeManager;

  // Static cache for standard Object methods to avoid re-resolving them
  private final Class<?>[] interfaces;
  private final ClassLoader cl;
  final long cleanup;
  final Map<Method, MethodDescriptor> methodCache;

  /**
   * Initializes the static cache for Object methods. This happens once when the
   * class is loaded.
   *
   * @param tm
   */
  public void init(TypeManager tm)
  {

  }

  public ProxyType(ProxyFactory factory, long cleanup, Class<?>[] interfaces)
  {

    this.context = factory.context;
    this.typeManager = context.getTypeManager();
    this.stringManager = context.getStringManager();
    this.interfaces = interfaces;
    this.cleanup = cleanup;
    this.builtin = factory.context.getBuildIn();

    // Pin the loader to the org.jpype module loader as the baseline default
    ClassLoader tempCl = ProxyType.class.getClassLoader();
    if (tempCl == null)
      tempCl = ClassLoader.getSystemClassLoader();

    // Only override if an interface explicitly comes from an independent classloader
    for (Class<?> cls : interfaces)
    {
      ClassLoader icl = cls.getClassLoader();
      // Ignore the bootstrap loader (null) and our own loader
      if (icl != null && icl != tempCl && icl != ClassLoader.getSystemClassLoader())
      {
        tempCl = icl;
        break; // Stop if we hit an explicit custom application/plugin loader
      }
    }
    this.cl = tempCl;

    // Build the instance-specific cache
    Map<Method, MethodDescriptor> tempMap = new HashMap<>();

    // 1. Bulk copy the pre-resolved Object methods
    tempMap.putAll(factory.objectMethods);

    // 2. Resolve interface-specific methods
    TypeManager tm = context.getTypeManager();
    synchronized (tm)
    {
      for (Class<?> iface : interfaces)
        populateCache(tm, iface.getMethods(), tempMap);
    }

    this.methodCache = Collections.unmodifiableMap(tempMap);
  }

  private void populateCache(TypeManager tm, Method[] methods, Map<Method, MethodDescriptor> map)
  {
    for (Method method : methods)
    {
      if (map.containsKey(method))
        continue;

      long returnType = tm.findClass(method.getReturnType());
      Class<?>[] params = method.getParameterTypes();
      long[] paramTypes = new long[params.length];
      for (int i = 0; i < params.length; i++)
        paramTypes[i] = tm.findClass(params[i]);

      boolean bypass = (method.isAnnotationPresent(Bypass.class));
      if (method.isAnnotationPresent(Builtin.class))
      {
        // Install a magic backdoor for builtin() so that Interface can find its support backend.
        try
        {
           MethodHandle defaultHandle = MethodHandles.lookup().findStatic(ProxyInstance.class, "get", MethodType.methodType(PyBuiltIn.class, Object.class));
           map.put(method, new MethodDescriptor(this.stringManager.get(method.getName()), returnType, paramTypes, defaultHandle, true));
           return;
        } catch (NoSuchMethodException | IllegalAccessException ex)
        {
          System.getLogger(ProxyType.class.getName()).log(System.Logger.Level.ERROR, (String) null, ex);
        }
      }

      MethodHandle defaultHandle = null;
      if (method.isDefault())
        defaultHandle = getDefaultHandle(method.getDeclaringClass(), method, java.lang.invoke.MethodHandles.class);
      map.put(method, new MethodDescriptor(this.stringManager.get(method.getName()), returnType, paramTypes, defaultHandle, bypass));
    }
  }

  public MethodDescriptor getMethodDescriptor(Method method)
  {
    return methodCache.get(method);
  }

  public Object newInstance(long instance)
  {
    ProxyInstance handler = new ProxyInstance(this, instance);
    Object proxy = Proxy.newProxyInstance(cl, interfaces, handler);
    context.getReferenceQueue().registerRef(proxy, instance, cleanup);
    return proxy;
  }

  native MethodHandle getDefaultHandle(Class<?> cls, Method method, Class<?> mhCls);

  @SuppressWarnings("unused")
  private static long unwrapPythonException(Throwable throwable)
  {
    if (throwable == null)
      return 0;
    if (throwable instanceof python.exceptions.PyBaseException)
      return unwrapObject(((python.exceptions.PyBaseException) throwable).get());
    if (throwable instanceof python.lang.PyExc)
      return unwrapObject(throwable);
    return 0;
  }

  private static long unwrapObject(Object obj)
  {
    if (!Proxy.isProxyClass(obj.getClass()))
      return 0;

    try
    {
      InvocationHandler handler = Proxy.getInvocationHandler(obj);
      if (handler instanceof ProxyInstance)
      {
        ProxyInstance jpypeHandler = (ProxyInstance) handler;
        return jpypeHandler.instance;
      }
    } catch (IllegalArgumentException e)
    {
    }
    return 0;
  }

  // exports to JNI
  @SuppressWarnings("unused")
  private static long getInstance(Object obj)
  {
    if (obj == null || !Proxy.isProxyClass(obj.getClass()))
      return 0L;
    InvocationHandler handler = Proxy.getInvocationHandler(obj);
    if (!(handler instanceof ProxyInstance))
      return 0L;
    ProxyInstance proxy = ((ProxyInstance) handler);
    return proxy.instance;
  }

}
