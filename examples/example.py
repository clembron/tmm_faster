import sys
from pathlib import Path

root_dir = Path(__file__).resolve().parent.parent
sys.path.append(str(root_dir))

import tmm_faster
import numpy as np
from matplotlib import pyplot as plt

wavelengths = np.linspace(200,1200,1000)
d = [np.inf, 5, 500, 5e4, 200, np.inf] 
c = ['i', 'c', 'c', 'i', 'c', 'i']
angles = np.linspace(0,90,1000)

n_list = [1.0, 0.05 + 3.0j, 2.4 + 0.001j, 1.5, 2.4 + 0.001j, 1.0]
n_array = np.array([n_list for _ in wavelengths])

res_incoherent = tmm_faster.calc_incoherent(n_array, d, c, angles, wavelengths)
res_coherent = tmm_faster.calc_coherent(n_array, d, angles, wavelengths)

fig, ax = plt.subplots(1,2,sharex=True,sharey=True,figsize=(10,4),constrained_layout=True)

ax[0].imshow(res_coherent['R_s'].T, extent=[wavelengths[0], wavelengths[-1], angles[0], angles[-1]],
                       aspect='auto', origin='lower', cmap='viridis', vmin=0, vmax=1)
ax[0].set_title('$R_s$ coherent')
ax[0].set_ylabel('Angle [°]')
im = ax[1].imshow(res_incoherent['R_s'].T, extent=[wavelengths[0], wavelengths[-1], angles[0], angles[-1]],
                       aspect='auto', origin='lower', cmap='viridis', vmin=0, vmax=1)
ax[1].set_title('$R_s$ incoherent')
for i in ax:
    i.set_xlabel('Wavelength [nm]')
cbar = fig.colorbar(im, ax=ax, location='right', label='Intensity', shrink=0.8)

fig.savefig(f'examples/example.png', dpi=300, bbox_inches='tight')
plt.show()