import clibtokamap

from abc import ABC, abstractmethod
import numpy as np

class DataSource(ABC):
    @abstractmethod
    def get(self, **kwargs: dict[str, str]) -> np.ndarray:
        ...

class Mapper:

    def __init__(self, mapping_path: str):
        self._mapper = clibtokamap.create(mapping_path)

    def register(self, name: str, data_source: DataSource):
        clibtokamap.register(self._mapper, name, data_source)

    def map(self, experiment: str, path: str, attributes: dict[str, str] | None = None):
        if attributes is None:
            attributes = {}
        return clibtokamap.map(self._mapper, experiment, path, attributes)
