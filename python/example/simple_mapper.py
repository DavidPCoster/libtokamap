from pathlib import Path
from typing import Any, override
import numpy as np
import json
import libtokamap


def dot_product(inputs: dict[str, np.array], _params: dict[str, Any]) -> np.array:
    if 'lhs' not in inputs:
        raise ValueError("lhs is required")
    if 'rhs' not in inputs:
        raise ValueError("rhs is required")
    lhs = inputs['lhs']
    rhs = inputs['rhs']
    if lhs.size != rhs.size:
        raise ValueError("lhs and rhs must have the same size")
    if lhs.ndim != 1 or rhs.ndim != 1:
        raise ValueError("lhs and rhs must be 1-dimensional")
    return np.array(np.dot(lhs, rhs))


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
        map(mapper, mapping, f"magnetics/coil[{coil}]/flux/dot_product")


def main(args):
    cxxlibs = False
    if len(args) == 2:
        if args[1] == "--help":
            print(f"Usage: python {args[0]} [--cxxlibs]")
            sys.exit(0)
        elif args[1] == "--cxxlibs":
            cxxlibs = True
        else:
            print(f"Usage: python {args[0]} [--cxxlibs]")
            sys.exit(1)

    print("Calling LibTokaMap version:", libtokamap.__version__)

    root = Path().absolute().parent
    build_root = root / "build" / "examples" / "simple_mapper"

    mapping_directory = root / "examples" / "simple_mapper" / "mappings"
    mapper = libtokamap.Mapper(str(mapping_directory))

    data_root = root / "examples" / "simple_mapper" / "data"

    if cxxlibs:
        build_root = root / "build" / "examples" / "simple_mapper"
        factory_library = build_root / ("libjson_data_source" + libtokamap.LibrarySuffix)
        mapper.register_data_source_factory("JSONFactory", str(factory_library))
        mapper.register_data_source("JSON", "JSONFactory", {"data_root": data_root})
        custom_function_library = build_root / ("libcustom_library" + libtokamap.LibrarySuffix)
        mapper.load_custom_function_library(custom_function_library)
    else:
        mapper.register_python_data_source("JSON", JSONDataSource(data_root))
        mapper.register_custom_function("custom", "dot_product", dot_product)

    mapping = "EXAMPLE"
    try:
        map_all(mapper, mapping)
    except Exception as e:
        print(f"{e}")


if __name__ == "__main__":
    import sys
    import timeit
    print(timeit.timeit("main(sys.argv)", number=10, setup="from __main__ import main"))
