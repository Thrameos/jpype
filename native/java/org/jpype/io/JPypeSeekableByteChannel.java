package org.jpype.io;

import org.jpype.JPypeContext;

public class JPypeSeekableByteChannel implements SeekableByteChannel
{
	PythonIOBase pyobj;

	public JPypeSeekableByteChannel(PythonIOBase pyobj)
	{
		check_(pyobj);
		this.pyobj = pyobj;
	}

	public long position()
	{
		return tell_(pyobj);
	}

	public long size()
	{
		return size_(pyobj);
	}

	public SeekableByteChannel position(long pos)
	{
		seek_(pyobj, pos);
		return this;
	}

	public SeekableByteChannel truncate(long size)
	{
		truncate_(pyobj, size);
	}

	public void read(ByteBuffer dst)
	{
		read_(pyobj, dst);
	}

	public int write(ByteBuffer src)
	{
		return write_(pyobj, src);
	}

	public boolean isOpen()
	{
		return isopen_(pyobj);
	}

	public void close()
	{
		return close_(pyobj);
	}

	static native void check_(long context, Object obj);
	static native  long tell_(long context, Object obj);
	static native void seek_(long context, Object obj, long pos);
	static native void read_(long context, Object obj, Object dst);
	static native int write_(long context, Object obj, Object src);
	static native void truncate_(long context, Object obj, long size);
	static native boolean isopen_(long context, Object obj);
	static native void close_(long context, Object obj);
}

