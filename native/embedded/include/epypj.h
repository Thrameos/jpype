#ifndef EPYJP_H
#define EPYJP_H
#include <jpype.h>
#include <pyjp.h>

void EJP_rethrow(JNIEnv *env, JPContext *context);

#ifdef JP_TRACING_ENABLE
#define EJP_TRACE_JAVA_IN(...) \
  JPContext* context = JPContext_global; \
  JPJavaFrame frame = JPJavaFrame::external(context, env); \
  JPPyCallAcquire callback; \
  JPypeTracer _trace(__VA_ARGS__); try {
#define EJP_TRACE_JAVA_OUT } \
  catch(...) { _trace.gotError(JP_STACKINFO()); EJP_rethrow(env, context); } \
  return X
#else

#define EJP_TRACE_JAVA_IN(...) \
  JPContext* context = JPContext_global; \
  JPJavaFrame frame = JPJavaFrame::external(context, env); \
  JPPyCallAcquire callback; \
  try {
#define EJP_TRACE_JAVA_OUT(X) } \
  catch(...) { EJP_rethrow(env, context); } return X
#endif

class EJPClass
{
public:
	jclass m_Wrapper;
	jmethodID m_Allocator;

	EJPClass(JPJavaFrame& frame, jclass wrapper);
	~EJPClass();
	static void destroy(PyObject *wrapper);

} ;

void EJP_Init(JPJavaFrame &frame);
void EJP_RegisterCalls(JPJavaFrame &frame);
bool EJP_HasPyType(PyTypeObject *type);
EJPClass *EJP_GetClass(JPJavaFrame &frame, PyTypeObject *type);
JPPyObject EJP_ToPython(JPJavaFrame& frame, jobject obj);
jobject EJP_ToJava(JPJavaFrame& frame, PyObject *obj, int flags);

#endif /* EPYJP_H */