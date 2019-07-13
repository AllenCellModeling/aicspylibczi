
class CziFileNotMultiSceneError(Exception):
    """
    This exception is intended to signal the user that the file being read has only 1 scene.
    The implementation of CziScene requires multiple scenes for the class to function. In the
    case of 1 scene only CziFile should be used to access the image.
    """
    pass
