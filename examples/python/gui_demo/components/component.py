import abc


class Component(metaclass=abc.ABCMeta):
    @abc.abstractmethod
    def refresh(self):
        pass
