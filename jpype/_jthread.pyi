class _JThread:
    @staticmethod
    def isAttached() -> bool: ...

    @staticmethod
    def attach() -> None: ...

    @staticmethod
    def attachAsDaemon() -> None: ...

    @staticmethod
    def detach() -> None: ...