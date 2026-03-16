import os
import sys

try:
    from . import tmm_faster_cpp
except ImportError:
    try:
        import tmm_faster.tmm_faster_cpp as tmm_faster_cpp
    except ImportError as e:
        raise ImportError(f"Could not find tmm_faster_cpp binary. Files present: {os.listdir(os.path.dirname(__file__))}") from e

core = tmm_faster_cpp.core
result = tmm_faster_cpp.result
__version__ = getattr(tmm_faster_cpp, "__version__", "0.1.0")

def calc_incoherent(n_matrix, d, c, angles, wavelengths):
    """
    Calculate incoherent Transfer Matrix Method (TMM) spectra.

    Computes reflection and transmission for a multilayer system
    where some or all layers are treated incoherently (ignoring 
    phase interference).

    Parameters
    ----------
    n_matrix : ndarray
        Complex refractive indices of the layers. 
        Shape should be [Wavelengths x Layers].
    d : array_like
        Thicknesses of each layer in nanometers (nm).
    c : array_like
        Coherence factors for each layer ('i'' for incoherent, 'c' for coherent).
    angles : array_like
        Incident angles in degrees.
    wavelengths : array_like
        Wavelengths in nanometers (nm).

    Returns
    -------
    results : dict
        A dictionary containing the 2D arrays:
        'R_avg', 'T_avg', 'R_s', 'R_p', 'T_s', 'T_p'.
    
    See Also
    --------
    calc_coherent : For systems where all layers are fully coherent.
    """
    return core().calc_incoherent(n_matrix, d, c, angles, wavelengths)

def calc_coherent(n_matrix, d, angles, wavelengths):
    """
    Calculate fully coherent Transfer Matrix Method (TMM) spectra.

    Computes reflection and transmission for a multilayer system where all 
    layers are thin enough to maintain phase coherence (interference effects 
    are included).

    Parameters
    ----------
    n_matrix : ndarray
        Complex refractive indices of the layers.
        Shape: [Wavelengths x Layers].
    d : array_like
        Thicknesses of each layer in nanometers (nm).
    angles : array_like
        Incident angles in degrees.
    wavelengths : array_like
        Wavelengths in nanometers (nm).

    Returns
    -------
    results : dict
        A dictionary containing the 2D arrays:
        'R_avg', 'T_avg', 'R_s', 'R_p', 'T_s', 'T_p'.

    Notes
    -----
    This method uses the standard 2x2 matrix formalism for coherent 
    multilayer optics. For thick substrates (e.g., glass slides), 
    `calc_incoherent` is usually more appropriate.

    """
    return core().calc_coherent(n_matrix, d, angles, wavelengths)

__all__ = ['core', 'result', 'calc_incoherent', 'calc_coherent']