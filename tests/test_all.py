import numpy as np
import pytest
from seam_carving import carve, compute_energy, ALGORITHMS
from seam_carving.carver import remove_seam

rng = np.random.default_rng(1)
IMG = rng.integers(0, 255, (14, 18, 3)).astype(np.uint8)


def seam_cost(e, s):
    return e[np.arange(len(s)), s].sum()


def test_energy_shape_and_flat_image_is_zero():
    assert compute_energy(IMG).shape == IMG.shape[:2]
    assert compute_energy(np.full((5, 5, 3), 7, np.uint8)).sum() == 0


@pytest.mark.parametrize("name", list(ALGORITHMS))
def test_seam_contract(name):
    e = compute_energy(IMG)
    s = ALGORITHMS[name](e)
    assert s.shape == (e.shape[0],)
    assert s.min() >= 0 and s.max() < e.shape[1]
    assert np.abs(np.diff(s)).max() <= 1


def brute_force_min(e):
    h, w = e.shape
    best = [np.inf]

    def rec(i, j, c):
        c += e[i, j]
        if i == h - 1:
            best[0] = min(best[0], c)
            return
        for d in (-1, 0, 1):
            if 0 <= j + d < w:
                rec(i + 1, j + d, c)

    for j in range(w):
        rec(0, j, 0)
    return best[0]


def test_dp_and_graphcut_are_optimal():
    e = rng.random((6, 7)) * 10
    opt = brute_force_min(e)
    assert seam_cost(e, ALGORITHMS["dp"](e)) == pytest.approx(opt)
    assert seam_cost(e, ALGORITHMS["graphcut"](e)) == pytest.approx(opt)


def test_greedy_never_beats_dp():
    for _ in range(20):
        e = rng.random((8, 9))
        assert seam_cost(e, ALGORITHMS["greedy"](e)) >= seam_cost(e, ALGORITHMS["dp"](e)) - 1e-9


def test_remove_seam():
    a = np.arange(12).reshape(3, 4)
    out = remove_seam(a, np.array([0, 1, 3]))
    assert out.tolist() == [[1, 2, 3], [4, 6, 7], [8, 9, 10]]


@pytest.mark.parametrize("name", list(ALGORITHMS))
def test_carve_sizes_and_seam_recording(name):
    res, st = carve(IMG, 15, 12, algorithm=name, record_seams=True)
    assert res.shape == (12, 15, 3)
    assert len(st["seams"]) == 5


# ---------- added in the completed version ----------
from seam_carving.algorithms import dp, greedy, graphcut, MAIN_ALGORITHMS
from seam_carving.energy import sobel_energy, sobel_energy_dc, forward_energy


def test_divide_and_conquer_energy_is_identical():
    big = rng.integers(0, 255, (37, 23, 3)).astype(np.uint8)
    for parts in (1, 2, 5, 40):
        assert np.array_equal(sobel_energy_dc(big, parts=parts), sobel_energy(big))
        assert np.array_equal(sobel_energy_dc(big, parts=parts, threads=3), sobel_energy(big))


def test_forward_energy_formula():
    img = np.array([[[0], [10], [30]]], dtype=np.uint8)
    assert forward_energy(img)[0].tolist() == [10, 30, 20]       # |I[j+1]-I[j-1]| with edge replication


def test_dp_variants_agree():
    e = rng.random((9, 11)) * 20
    c = seam_cost(e, dp.find_seam(e))
    assert seam_cost(e, dp.find_seam_loop(e)) == pytest.approx(c)
    assert seam_cost(e, dp.find_seam_low_memory(e)) == pytest.approx(c)


def test_all_maxflow_engines_find_the_optimum():
    e = rng.random((6, 8)) * 10
    opt = brute_force_min(e)
    for f in (graphcut.find_seam, graphcut.find_seam_edmonds_karp, graphcut.find_seam_networkx):
        assert seam_cost(e, f(e)) == pytest.approx(opt)


def test_count_seams_is_exponential():
    assert dp.count_seams(10, 2) == 2 ** 10
    for m in range(2, 12):
        assert dp.count_seams(m, 3) >= 2 ** m


def test_greedy_failure_case():
    E = np.array([[0, 9, 1, 9, 9]] + [[50, 50, 1, 50, 50]] * 3, dtype=float)
    assert seam_cost(E, greedy.find_seam(E)) == 150
    assert seam_cost(E, dp.find_seam(E)) == 4


def test_lookahead_improves_on_greedy_in_failure_case():
    E = np.array([[0, 9, 1, 9, 9]] + [[50, 50, 1, 50, 50]] * 3, dtype=float)
    assert seam_cost(E, greedy.find_seam_lookahead(E, 3)) == 4


def test_alternate_order_and_energy_methods():
    for order in ("vertical_first", "alternate"):
        res, st = carve(IMG, 12, 9, algorithm="dp", order=order, energy_method="forward")
        assert res.shape == (9, 12, 3)


def test_seams_cover_distinct_original_pixels():
    _, st = carve(IMG, 14, 10, algorithm="dp", record_seams=True, order="alternate")
    pts = {tuple(p) for _, s in st["seams"] for p in s}
    assert len(pts) == sum(len(s) for _, s in st["seams"])      # no pixel removed twice


def test_bad_target_rejected():
    with pytest.raises(ValueError):
        carve(IMG, 99, 5)
