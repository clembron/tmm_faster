import numpy as np
from typing import List, Dict, Any, Union

# Definition der Ergebnis-Struktur
class result:
    """
    Container for TMM calculation results for a single wavelength/angle point.
    """
    R_avg: float
    T_avg: float
    R_s: float
    R_p: float
    T_s: float
    T_p: float

# Definition der Hauptklasse
class core:
    """
    Fast TMM (Transfer-Matrix Method) core class with OpenMP parallelization.
    """
    
    def __init__(self) -> None:
        """Initializes a new TMM core instance."""
        ...

    def calc_coherent_single(
        self, 
        n: List[complex], 
        d: List[float], 
        th_0: float, 
        lam: float
    ) -> result:
        """
        Calculates R/T for a stack of coherent layers for a single point.

        Parameters
        ----------
        n : list of complex
            Complex refractive indices for each layer.
        d : list of float
            Thicknesses in nanometers.
        th_0 : float
            Incident angle in DEGREES.
        lam : float
            Vacuum wavelength in nanometers.

        Returns
        -------
        res : result
            An object containing R and T values for all polarizations.
        """
        ...

    def calc_incoherent_single(
        self, 
        n: List[complex], 
        d: List[float], 
        c: List[str], 
        th_0: float, 
        lam: float
    ) -> result:
        """
        Calculates R/T for a system including incoherent layers for a single point.

        Parameters
        ----------
        n : list of complex
            Complex refractive indices for each layer.
        d : list of float
            Thicknesses in nanometers.
        c : list of str
            Coherence characters ('c' for coherent, 'i' for incoherent).
        th_0 : float
            Incident angle in DEGREES.
        lam : float
            Vacuum wavelength in nanometers.

        Returns
        -------
        res : result
            An object containing R and T values for all polarizations.
        """
        ...

    def calc_incoherent(
        self, 
        n_matrix: Union[np.ndarray, List[List[complex]]], 
        d: List[float], 
        c: List[str], 
        angles: List[float], 
        wavelengths: List[float]
    ) -> Dict[str, np.ndarray]:
        """
        High-performance grid calculation for systems with coherent and incoherent layers.
        Utilizes OpenMP for multi-core processing.

        Parameters
        ----------
        n_matrix : ndarray
            Complex refractive index matrix [Wavelengths x Layers].
        d : list of float
            Layer thicknesses in nanometers.
        c : list of str
            Coherence status ('c' or 'i') per layer.
        angles : list of float
            Incident angles in degrees.
        wavelengths : list of float
            Vacuum wavelengths in nanometers.

        Returns
        -------
        results : dict
            A dictionary containing the 2D arrays:
            'R_avg', 'T_avg', 'R_s', 'R_p', 'T_s', 'T_p'.
        """
        ...

    def calc_coherent(
        self, 
        n_matrix: Union[np.ndarray, List[List[complex]]], 
        d: List[float], 
        angles: List[float], 
        wavelengths: List[float]
    ) -> Dict[str, np.ndarray]:
        """
        High-performance grid calculation for systems with coherent layers.
        Utilizes OpenMP for multi-core processing.

        Parameters
        ----------
        n_matrix : ndarray
            Complex refractive index matrix [Wavelengths x Layers].
        d : list of float
            Layer thicknesses in nanometers.
        angles : list of float
            Incident angles in degrees.
        wavelengths : list of float
            Vacuum wavelengths in nanometers.

        Returns
        -------
        results : dict
            A dictionary containing the 2D arrays:
            'R_avg', 'T_avg', 'R_s', 'R_p', 'T_s', 'T_p'.
        """
        ...