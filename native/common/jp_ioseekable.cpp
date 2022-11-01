extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_io_JPypeSeekableByteChannel_check_(
		JNIEnv *env, jclass clazz,
		jobject proxy)
{
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		// Get the target for the proxy
		PyJPProxy *target = ((JPProxy*) frame.GetLongField(proxy, context->m_Target->m_InstanceID))->m_Instance;
		JPPyObject pyobj = JPPyObject::use(target->m_Target);

		// Check if it is an instance
		PyObject *typing = PyImport_AddModule("io");
		JPPyObject proto = JPPyObject::call(PyObject_GetAttrString(typing, "IOBase"));
		if (PyObject_IsInstance(pyobj.get(), proto.get()) == 0)
		{
				env->functions->ThrowNew(env, context->m_RuntimeException.get(),
						"object is not a Python IO handle");
				return NULL;
		}
	}
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_io_JPypeSeekableByteChannel_tell_(
		JNIEnv *env, jclass clazz,
		jobject proxy)
{
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		// Get the target for the proxy
		PyJPProxy *target = ((JPProxy*) frame.GetLongField(proxy, context->m_Target->m_InstanceID))->m_Instance;
		JPPyObject pyobj = JPPyObject::use(target->m_Target);
		JPPyObject ret = JPPyObject::call(PyObject_CallMethod(pyobj.get(), "tell"));
		// FIXME check return type, can be none or an exception
		return PyLong_AsLong(ret.get());
	}
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_io_JPypeSeekableByteChannel_seek_(
		JNIEnv *env, jclass clazz,
		jobject proxy, 
		jlong pos)
{
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		// Get the target for the proxy
		PyJPProxy *target = ((JPProxy*) frame.GetLongField(proxy, context->m_Target->m_InstanceID))->m_Instance;
		JPPyObject pyobj = JPPyObject::use(target->m_Target);
		JPPyObject ret = JPPyObject::call(PyObject_CallMethod(pyobj.get(), "seek", "l", pos));
		// FIXME check return type, can be none or an exception
	}
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_io_JPypeSeekableByteChannel_truncate_(
		JNIEnv *env, jclass clazz,
		jobject proxy, 
		jlong pos)
{
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		// Get the target for the proxy
		PyJPProxy *target = ((JPProxy*) frame.GetLongField(proxy, context->m_Target->m_InstanceID))->m_Instance;
		JPPyObject pyobj = JPPyObject::use(target->m_Target);
		JPPyObject ret = JPPyObject::call(PyObject_CallMethod(pyobj.get(), "truncate", "l", pos));
		// FIXME check return type, can be none or an exception
	}
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_io_JPypeSeekableByteChannel_read_(
		JNIEnv *env, jclass clazz,
		jobject proxy, 
		jobject buf)
{
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		// Get the target for the proxy
		PyJPProxy *target = ((JPProxy*) frame.GetLongField(proxy, context->m_Target->m_InstanceID))->m_Instance;
		JPPyObject pyobj = JPPyObject::use(target->m_Target);
		JPPyObject ret = JPPyObject::call(PyObject_CallMethod(pyobj.get(), "truncate", "l", pos));
		// FIXME check return type, can be none or an exception
	}
}

extern "C" JNIEXPORT jobject JNICALL Java_org_jpype_io_JPypeSeekableByteChannel_write_(
		JNIEnv *env, jclass clazz,
		jobject proxy, 
		jobject buf)
{
	JPContext* context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::external(context, env);

	// We need the resources to be held for the full duration of the proxy.
	JPPyCallAcquire callback;
	{
		// Get the target for the proxy
		PyJPProxy *target = ((JPProxy*) frame.GetLongField(proxy, context->m_Target->m_InstanceID))->m_Instance;
		JPPyObject pyobj = JPPyObject::use(target->m_Target);
		JPPyObject ret = JPPyObject::call(PyObject_CallMethod(pyobj.get(), "truncate", "l", pos));
		// FIXME check return type, can be none or an exception
	}
}




