from pathlib import Path
from typing import override
import numpy as np
import json
import libtokamap
from quopri import decode

class JSONDataSource(libtokamap.DataSource):
    def __init__(self, data_root: Path):
        self.data_root = data_root

    @override
    def get(self, args: dict[str, str]) -> np.ndarray:
        if 'file_name' not in args:
            raise ValueError("file_name is required")
        if 'signal' not in args:
            raise ValueError("signal is required")

        file_name = args['file_name']
        signal = args['signal']

        with open(self.data_root / file_name, 'r') as f:
            data = json.load(f)

        tokens = signal.split('/')
        for token in tokens:
            try:
                num = int(token)
                data = data[num]
            except ValueError:
                data = data[token]

        return np.array(data)


def map(mapper: libtokamap.Mapper, mapping: str, signal: str):
    res = mapper.map(mapping, signal, {'shot': 42})
    if res.dtype == 'S1':
        res = res.tobytes().decode()
    print(f"{signal}: {res}")
    return res


def map_all(mapper: libtokamap.Mapper, mapping: str):
    map(mapper, mapping, "magnetics/version")
    n_coils = map(mapper, mapping, "magnetics/coil")
    for coil in range(n_coils):
        map(mapper, mapping, f"magnetics/coil[{coil}]/name")
        num_positions = map(mapper, mapping, f"magnetics/coil[{coil}]/position")
        for position in range(num_positions):
            map(mapper, mapping, f"magnetics/coil[{coil}]/position[{position}]/r")
            map(mapper, mapping, f"magnetics/coil[{coil}]/position[{position}]/z")
        map(mapper, mapping, f"magnetics/coil[{coil}]/flux/time")
        map(mapper, mapping, f"magnetics/coil[{coil}]/flux/data")


def main():
    root = Path().absolute().parent
    mapping_directory = root / "examples" / "simple_mapper" / "mappings"
    mapper = libtokamap.Mapper(str(mapping_directory))

    data_root = root / "examples" / "simple_mapper" / "data"
    mapper.register("JSON", JSONDataSource(data_root))

    mapping = "EXAMPLE"
    try:
        map_all(mapper, mapping)
    except Exception as e:
        print(f"{e}")

if __name__ == "__main__":
    main()
