import csv, sys, math, statistics as st
from collections import defaultdict
T95={9:2.262,8:2.306,7:2.365,6:2.447,5:2.571,4:2.776,3:3.182,2:4.303,1:12.706}
d=defaultdict(dict)
for r in csv.DictReader(open(sys.argv[1])):
    d[(int(r['threads']),r['side'])][int(r['round'])]=float(r['run_ms'])
cv=lambda xs: 100*st.stdev(xs)/st.mean(xs)
rows=[]; roundratios=defaultdict(list)
for t in range(1,9):
    b=d[(t,'base')]; f=d[(t,'full')]
    rs=sorted(set(b)&set(f))
    lr=[math.log(f[r]/b[r]) for r in rs]
    for r,x in zip(rs,lr): roundratios[r].append(x)
    n=len(rs); mu=st.mean(lr); h=T95[n-1]*st.stdev(lr)/math.sqrt(n)
    rows.append(dict(threads=t,base_median=st.median(b.values()),full_median=st.median(f.values()),
        base_min=min(b.values()),full_min=min(f.values()),base_cv=cv(list(b.values())),full_cv=cv(list(f.values())),
        delta_pct=100*(math.exp(mu)-1),ci_lo=100*(math.exp(mu-h)-1),ci_hi=100*(math.exp(mu+h)-1),
        min_ratio_pct=100*(min(f[r]/b[r] for r in rs)-1),max_ratio_pct=100*(max(f[r]/b[r] for r in rs)-1),n=n))
keys=list(rows[0].keys())
with open(sys.argv[2],'w') as fo:
    w=csv.writer(fo); w.writerow(keys)
    for s in rows: w.writerow([f"{s[k]:.3f}" if isinstance(s[k],float) else s[k] for k in keys])
print("| t | base median ms | [0,1] median ms | change % | 95% CI | per-round range % | CV base/[0,1] % |")
print("|---|---|---|---|---|---|---|")
for s in rows:
    print(f"| {s['threads']} | {s['base_median']:.1f} | {s['full_median']:.1f} | {s['delta_pct']:+.1f} | [{s['ci_lo']:+.1f}, {s['ci_hi']:+.1f}] | {s['min_ratio_pct']:+.1f} to {s['max_ratio_pct']:+.1f} | {s['base_cv']:.1f} / {s['full_cv']:.1f} |")
print("\nper-round geomean ratio full/base over all thread counts (%):")
for r in sorted(roundratios): print(r, f"{100*(math.exp(st.mean(roundratios[r]))-1):+.2f}")
