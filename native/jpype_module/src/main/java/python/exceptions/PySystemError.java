// --- file: python/exceptions/PySystemError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PySystemError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PySystemError(PyExc base)
  {
    super(base);
  }
}
