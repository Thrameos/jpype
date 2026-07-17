.. _managing_crossplatform_gui_environments:

Managing Cross-Platform GUI Environments
****************************************

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) GUI-environment
story -- ``python.io``/``python.collections``/``python.datetime`` don't
expose a GUI-toolkit front end, so no ``gui_java.rst`` is planned. If that
changes, this note should be replaced with a cross-reference.

JPype provides utility functions, `setupGuiEnvironment` and
`shutdownGuiEnvironment`, to manage GUI environments across platforms,
ensuring compatibility with macOS, Linux, and Windows. These functions are
particularly useful for Swing and JavaFX-based applications, where macOS
imposes specific requirements for GUI event loops. Even on Linux and Windows,
using `setupGuiEnvironment` ensures consistent behavior and avoids potential
issues with threading and event loops.

.. _managing_crossplatform_gui_environments_setupguienvironmentcb:

setupGuiEnvironment(cb)
=======================

**Description**:

`setupGuiEnvironment` ensures that GUI applications can run correctly across
all platforms. It is specifically designed to address macOS's requirement for
the main thread to run the event loop, but it is also recommended for Swing
and JavaFX applications on Linux and Windows to maintain cross-platform
compatibility and proper threading behavior.

**Parameters**:

- **cb**: A callback function that initializes and launches the GUI application.

**Behavior**:

- **macOS**:
  - Creates a Java thread using a `Runnable` proxy.
  - Starts the macOS event loop using `PyObjCTools.AppHelper.runConsoleEventLoop()`.

- **Other Platforms (Linux, Windows)**:
  - Executes the callback function directly.

**Why Use This Function for Swing and JavaFX Applications?**

Swing and JavaFX applications often rely on proper threading and event loop
management to function correctly. While macOS has strict requirements for
running the event loop on the main thread, using `setupGuiEnvironment` on
Linux and Windows ensures consistent behavior and avoids potential threading
issues, such as race conditions or improper GUI updates.

**Example**:

.. code-block:: python

    from jpype import setupGuiEnvironment
    from javafx.application import Platform

    def say_hello_later():
        """Test function for scheduling a task on the JavaFX Application Thread."""
        print("Hello from JavaFX!")

    def launch_gui():
        """Launch the GUI application."""
        # Example: Schedule a task on the JavaFX Application Thread
        Platform.runLater(say_hello_later)
        print("GUI launched")

    # Use setupGuiEnvironment to ensure cross-platform compatibility
    setupGuiEnvironment(launch_gui)

.. _managing_crossplatform_gui_environments_reestablishing_an_interactive_shell_on_another_thread:

Reestablishing an Interactive Shell on Another Thread
=====================================================

When using `setupGuiEnvironment`, the main thread may be occupied by the GUI
event loop (particularly on macOS). To allow interactive debugging in Python,
you can launch an interactive shell (e.g., IPython) on a separate thread.

**Steps**:

1. Use `setupGuiEnvironment` to start the GUI application.
2. Launch an interactive shell on a separate thread using Python's `threading`
   module.

**Example**:

.. code-block:: python

    import threading
    import IPython

    def launch_interactive_shell():
        """Launch an interactive shell on a separate thread."""
        IPython.embed()

    # Start the interactive shell on another thread
    thread = threading.Thread(target=launch_interactive_shell)
    thread.start()

By combining this approach with `setupGuiEnvironment`, you can interact with
the Python environment while the GUI application is running.

.. _managing_crossplatform_gui_environments_shutdownguienvironment:

shutdownGuiEnvironment()
========================

**Description**:

`shutdownGuiEnvironment` is used to cleanly terminate the macOS event loop. On
other platforms, it performs no action.

**Behavior**:

- **macOS**:
  - Stops the macOS event loop using `PyObjCTools.AppHelper.stopEventLoop()`.

- **Other Platforms (Linux, Windows)**:
  - No action is taken.

**Example**:

.. code-block:: python

    from jpype import shutdownGuiEnvironment

    # Shutdown the GUI environment (macOS-specific)
    shutdownGuiEnvironment()

.. _managing_crossplatform_gui_environments_best_practices_on_guis:

Best Practices on GUIs
--------------------------

- **Use `setupGuiEnvironment` for All Platforms**:
  Even though macOS has specific requirements, using `setupGuiEnvironment`
  ensures consistent behavior across all platforms, particularly for Swing
  and JavaFX applications.

- **Thread Safety**:
  Always schedule GUI updates using JavaFX's `Platform.runLater` or Swing's
  `SwingUtilities.invokeLater` to ensure they occur on the appropriate thread.

- **Interactive Debugging**:
  Launch an interactive shell on a separate thread for debugging while the GUI
  application is running.

- **Exception Handling**:
  Wrap callback functions in `try-except` blocks to prevent unhandled
  exceptions from disrupting the GUI.

- **Cross-Platform Testing**:
  Test the application on macOS, Linux, and Windows to ensure compatibility.

.. _managing_crossplatform_gui_environments_summary_of_guis:

Summary of GUIs
===============

The `setupGuiEnvironment` function is a critical tool for managing GUI
environments across platforms, particularly for Swing and JavaFX-based
applications. It ensures compatibility with macOS's event loop requirements
while maintaining simplicity on other platforms. Combined with the ability to
launch an interactive shell on a separate thread, this approach provides a
robust solution for developing and debugging GUI applications in Python.
