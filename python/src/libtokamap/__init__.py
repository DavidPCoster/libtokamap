import clibtokamap

from abc import ABC, abstractmethod
import numpy as np


class DataSource(ABC):
    """Abstract base class for data sources.

    Subclasses should implement the `get` method to retrieve data. Any attempt to register a data source that
    is not a subclass of DataSource will raise a Exception.
    """
    @abstractmethod
    def get(self, args: dict[str, str]) -> np.ndarray:
        """Get the data from the data source.

        Args:
            args: A dictionary of arguments provided by the mapping which can be used to identify the data to retrieve.

        Returns:
            A numpy array containing the data.
        """
        ...

class Mapper:
    """Mapper class for mapping data from data sources.

    This class provides a way to map data from multiple data sources based on a mapping configuration.
    """

    def __init__(self, mapping_path: str):
        """Initialize a new Mapper instance.

        Args:
            mapping_path: The path to the directory containing the mapping files.
        """
        self._mapper = clibtokamap.create(mapping_path)

    def register(self, name: str, data_source: DataSource) -> None:
        """Register a data source with the mapper.

        Args:
            name: The name of the data source.
            data_source: The data source to register.

        Throws:
            LibTokaMapError: If the data source is not a subclass of DataSource.
        """
        clibtokamap.register(self._mapper, name, data_source)

    def map(self, experiment: str, path: str, attributes: dict[str, str] | None = None) -> np.ndarray:
        """Map the data from the data sources.

        Args:
            experiment: The name of the experiment.
            path: The path to the data.
            attributes: A dictionary of attributes to pass to the data sources.

        Returns:
            A numpy array containing the mapped data.
        """
        if attributes is None:
            attributes = {}
        return clibtokamap.map(self._mapper, experiment, path, attributes)
