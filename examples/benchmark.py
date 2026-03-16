import sys
from pathlib import Path
root_dir = Path(__file__).resolve().parent.parent
sys.path.append(str(root_dir))

import tmm_fast
import tmm_faster
import numpy as np
import pandas as pd
import torch
import time

NUM_WAVELENGTHS = np.geomspace(1, 1e6, num=7)
NUM_LAYERS = [10]
NUM_TOTAL_RUNS = 2
NUM_REPS = 5

def measure_runtime(func, repeats=10, loops_per_repeat=1, device='cpu'):

    costs = []
    for _ in range(repeats):
        if device == 'cuda' and torch.cuda.is_available():
            torch.cuda.synchronize()
        
        start = time.perf_counter()
        for _ in range(loops_per_repeat):
            func()
            
        if device == 'cuda' and torch.cuda.is_available():
            torch.cuda.synchronize()
        
        end = time.perf_counter()
        costs.append((end - start) / loops_per_repeat)
    
    return np.min(costs) * 1000  # in ms

def benchmark(num_layers, num_wavelengths, num_angles):

    wavelengths = np.linspace(200, 1200, num_wavelengths)    
    wavelengths_fast = torch.from_numpy(wavelengths) * (1e-9)

    angles = np.linspace(45, 90, num_angles)    
    angles_fast = torch.from_numpy(angles) * (torch.pi / 180)

    d = np.random.uniform(1, 1000, size=num_layers)
    d = np.concatenate(([np.inf], d, [np.inf]))
    d_fast = torch.tensor(d)[None, :] * 1e-9

    n_real = np.random.uniform(1, 5, size=num_layers)
    n_imag = np.random.beta(a=1, b=4, size=num_layers) * 2
    n_array = n_real + 1j * n_imag
    n_array = np.concatenate(([1], n_array, [1]))
    n_array = np.array([n_array for _ in wavelengths])
    n_torch = torch.from_numpy(n_array).T[None]

    def run_fast():
        tmm_fast.coh_tmm('s', n_torch, d_fast, angles_fast, wavelengths_fast, device='cpu')
        # tmm_fast.coh_tmm('p', n_torch, d_fast, angles_fast, wavelengths_fast, device='cpu')

    def run_fast_cuda():
        n_c = n_torch.to('cuda')
        d_c = d_fast.to('cuda')
        a_c = angles_fast.to('cuda')
        w_c = wavelengths_fast.to('cuda')
        return lambda: tmm_fast.coh_tmm('s', n_c, d_c, a_c, w_c, device='cuda')

    def run_faster():
        tmm_faster.calc_coherent(n_array, d, angles, wavelengths)

    # tmm_fast - cpu
    min_fast = measure_runtime(run_fast, repeats=NUM_REPS, device='cpu')

    # tmm_fast - cuda
    min_fast_cuda = np.nan
    if torch.cuda.is_available():
        try:
            cuda_func = run_fast_cuda()
            # Warmup CUDA
            cuda_func() 
            min_fast_cuda = measure_runtime(cuda_func, repeats=NUM_REPS, device='cuda')
        except Exception as e:
            print(f"CUDA Error: {e}")
    
    # tmm_faster
    loops = 100 if num_wl < 1000 else 1
    min_faster = measure_runtime(run_faster, repeats=NUM_REPS, loops_per_repeat=loops, device='cpu')    

    print(f"Layers: {num_layers}, WL: {num_wavelengths:7d} | "
          f"tmm_fast (CPU): {min_fast:8.4f}ms | tmm_fast (CUDA): {min_fast_cuda:8.4f}ms | "
          f"tmm_faster: {min_faster:8.4f}ms | ")

    return min_fast, min_fast_cuda, min_faster

results = []
int_list_wl = sorted(list(set(NUM_WAVELENGTHS.astype(int))))

# run benchmark
for i in range(NUM_TOTAL_RUNS):
    print(f'RUN #{i+1}/{NUM_TOTAL_RUNS}')
    for num_layers in NUM_LAYERS:
        for num_wl in int_list_wl:
            res = benchmark(num_layers, num_wl, 1)
            results.append((num_layers, num_wl, res))

data = []
for n_layers, n_wl, (t_fast, t_fast_cuda, t_faster) in results:
    data.append({
        'num_layers': n_layers,
        'num_wl': n_wl,
        't_fast': t_fast,
        't_fast_cuda': t_fast_cuda,
        't_faster': t_faster,
        'speedup_cpu': t_fast / t_faster if t_faster > 0 else 0,
        'speedup_cuda': t_fast_cuda / t_faster if (not np.isnan(t_fast_cuda) and t_faster > 0) else 0
    })

df = pd.DataFrame(data)
df_min = df.groupby(['num_layers', 'num_wl']).min().reset_index()
print(df_min)