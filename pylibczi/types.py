from io import BufferedIOBase, BufferedReader
from pathlib import Path
from typing import Union

# IO Types
PathLike = Union[str, Path]
BufferLike = Union[bytes, BufferedIOBase, BufferedReader]
FileLike = Union[PathLike, BufferLike]