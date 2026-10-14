import csv, sys, math, statistics as st
from collections import defaultdict
T95={9:2.262}
d=defaultdict(dict)
for r in csv.DictReader(open(sys.argv[1])):
    d[(r['scene'],int(r['threads']),r['side'])][int(r['round'])]=float(r['run_ms'])
scenes=sorted({k[0] for k in d})
cv=lambda xs: 100*st.stdev(xs)/st.mean(xs)
summ=[]; roundratios=defaultdict(list)
for sc in scenes:
    for t in range(1,9):
        m=d[(sc,t,'master')]; a=d.get((sc,t,'main'))
        if a is None:
            v=list(m.values()); summ.append(dict(scene=sc,threads=t,main_median='',master_median=st.median(v),master_min=min(v),master_cv=cv(v),n=len(v)))
            continue
        rs=sorted(m)
        lr=[math.log(m[r]/a[r]) for r in rs]
        for r,x in zip(rs,lr): roundratios[r].append(x)
        n=len(rs); mu=st.mean(lr); h=T95[n-1]*st.stdev(lr)/math.sqrt(n)
        summ.append(dict(scene=sc,threads=t,main_median=st.median(a.values()),master_median=st.median(m.values()),
          main_min=min(a.values()),master_min=min(m.values()),main_cv=cv(list(a.values())),master_cv=cv(list(m.values())),
          delta_pct=100*(math.exp(mu)-1),ci_lo=100*(math.exp(mu-h)-1),ci_hi=100*(math.exp(mu+h)-1),n=n))
keys=['scene','threads','main_median','master_median','main_min','master_min','main_cv','master_cv','delta_pct','ci_lo','ci_hi','n']
with open(sys.argv[2],'w') as f:
    w=csv.writer(f); w.writerow(keys)
    for s in summ: w.writerow([f"{s.get(k,''):.3f}" if isinstance(s.get(k,''),float) else s.get(k,'') for k in keys])
# round drift
print("per-round geomean ratio master/main over all paired cells (%):")
for r in sorted(roundratios): print(r, f"{100*(math.exp(st.mean(roundratios[r]))-1):+.2f}")
# overall
print("\nper-thread geomean across paired scenes:")
for t in range(1,9):
    xs=[math.log(s['master_median']/s['main_median']) for s in summ if s['threads']==t and s['main_median']!='']
    print(t,f"{100*(math.exp(st.mean(xs))-1):+.2f}")
print("\nsignificant (CI excludes 0):")
for s in summ:
    if 'ci_lo' in s and (s['ci_lo']>0 or s['ci_hi']<0): print(s['scene'],s['threads'],f"{s['delta_pct']:+.1f} [{s['ci_lo']:+.1f},{s['ci_hi']:+.1f}] cv {s['main_cv']:.1f}/{s['master_cv']:.1f}")
print("\nmax cv:",max(max(s.get('main_cv',0),s['master_cv']) for s in summ))
