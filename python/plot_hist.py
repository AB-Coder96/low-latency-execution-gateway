#!/usr/bin/env python3
import argparse
import os
import pandas as pd
import matplotlib.pyplot as plt

def percentiles(series):
    qs = [0.50, 0.95, 0.99, 0.999]
    out = series.quantile(qs, interpolation="linear")
    return {f"p{int(q*1000)/10:g}": float(out.loc[q]) for q in qs}

def main():
    ap = argparse.ArgumentParser(description="Plot latency histogram from ZetaLatency CSV output.")
    ap.add_argument("csv", help="Input CSV (output of ZetaLatency_bench)")
    ap.add_argument("--col", default=None, help="Column to plot (default: latency_ns or end_to_end_ns)")
    ap.add_argument("--bins", type=int, default=200, help="Histogram bins (default 200)")
    ap.add_argument("--out", default=None, help="Output PNG path (default: <csv>.png)")
    args = ap.parse_args()

    df = pd.read_csv(args.csv, comment="#")
    if df.empty:
        raise SystemExit("No samples found (CSV has no data rows).")

    col = args.col
    if col is None:
        if "latency_ns" in df.columns:
            col = "latency_ns"
        elif "end_to_end_ns" in df.columns:
            col = "end_to_end_ns"
        else:
            col = df.columns[0]

    s = df[col].astype("int64")
    p = percentiles(s)

    out_png = args.out or (args.csv + ".png")
    plt.figure()
    plt.hist(s, bins=args.bins)
    plt.title(f"{os.path.basename(args.csv)} — {col}\n"
              f"p50={p['p50']} ns  p95={p['p95']} ns  p99={p['p99']} ns  p99.9={p['p99.9']} ns")
    plt.xlabel("latency (ns)")
    plt.ylabel("count")
    plt.tight_layout()
    plt.savefig(out_png, dpi=150)

    print("Percentiles (ns):", p)
    print("Saved:", out_png)

if __name__ == "__main__":
    main()
