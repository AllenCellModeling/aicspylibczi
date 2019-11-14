from io import BufferedIOBase, BufferedReader
from pathlib import Path
from typing import BinaryIO, Union

# IO Types
PathLike = Union[str, Path]
#BufferLike: object = Union[bytes, BinaryIO, BufferedIOBase, BufferedReader]
BufferLike: object = Union[bytes, BufferedIOBase, BufferedReader]
FileLike = Union[PathLike, BufferLike]
