import json, sys
s=open('model.js').read(); M=json.loads(s[s.find('{'):s.rfind('}')+1])
SYM='^-0125'; OUTSYM=['0','1','2','5']
INSTR=[0,1,2,7,6,11,10,4,9]   # ac, bd, sd, cp, rs, ch, oh, mt, cy
def build(style):
    nodes=[]  # each: dict children sym->idx, dist or None
    def new(): nodes.append({'ch':{},'d':None}); return len(nodes)-1
    roots=[]
    for i in INSTR:
        r=new(); roots.append(r)
        for ctx,dist in M[style][i].items():
            n=r
            for c in reversed(ctx):
                if c not in nodes[n]['ch']: nodes[n]['ch'][c]=new()
                n=nodes[n]['ch'][c]
            nodes[n]['d']=dist
    # BFS relayout so children contiguous
    order=[]; idx={}
    q=list(roots)
    for r in roots: idx[r]=len(order); order.append(r)
    while q:
        n=q.pop(0)
        for c in sorted(nodes[n]['ch'], key=SYM.index):
            k=nodes[n]['ch'][c]; idx[k]=len(order); order.append(k); q.append(k)
    out=[]
    for n in order:
        nd=nodes[n]; ch=sorted(nd['ch'].items(), key=lambda kv:SYM.index(kv[0]))
        first=idx[ch[0][1]] if ch else 0
        # cumulative thresholds out of 255 for 0,1,2 ; 5 is remainder
        if nd['d']:
            acc=0; th=[]
            for sname in ['0','1','2']:
                acc+=nd['d'].get(sname,0.0); th.append(min(255,int(round(acc*255))))
            has=1
        else: th=[0,0,0]; has=0
        mask=0
        for c,_ in ch: mask|=1<<SYM.index(c)
        out.append((first,mask,has,th))
    # also need symbol of each child to map index: children sorted by SYM order, mask tells which present
    return [idx[r] for r in roots], out
def emit(style, f):
    roots,nodes=build(style)
    assert len(nodes)<65536
    f.write('/* generated from DrumBrain model.js, style %s: %d nodes */\n'%(style,len(nodes)))
    f.write('#include <stdint.h>\n#include "dbmodel.h"\n')
    f.write('const uint16_t db_roots[DB_NINSTR]={%s};\n'%','.join(map(str,roots)))
    f.write('const struct db_node db_nodes[%d]={\n'%len(nodes))
    for first,mask,has,th in nodes:
        f.write('{%d,%d,%d,{%d,%d,%d},0},\n'%(first,mask,has,*th))
    f.write('};\n')
    return len(nodes)

STYLES=['house','breaks']
def emit_all(f):
    f.write('/* generated from DrumBrain model.js: styles house, breaks */\n#include <stdint.h>\n#include "dbmodel.h"\n')
    allnodes=[]; roots=[]
    for st in STYLES:
        r,nodes=build(st); off=len(allnodes)
        roots.append([x+off for x in r])
        for first,mask,has,th in nodes:
            allnodes.append((first+off if mask else 0,mask,has,th))
    assert len(allnodes)<65536
    f.write('const uint16_t db_roots[DB_NSTYLES][DB_NINSTR]={%s};\n'%','.join('{'+','.join(map(str,r))+'}' for r in roots))
    f.write('const struct db_node db_nodes[%d]={\n'%len(allnodes))
    for first,mask,has,th in allnodes: f.write('{%d,%d,%d,{%d,%d,%d},0},\n'%(first,mask,has,*th))
    f.write('};\n'); return len(allnodes)
print(emit_all(open('dbmodel.c','w')),'nodes')
