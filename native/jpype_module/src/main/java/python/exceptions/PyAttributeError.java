// --- file: python/exceptions/PyAttributeError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyAttributeError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyAttributeError(PyExc base)
  {
    super(base);
  }
}
