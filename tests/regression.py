from pathlib import Path
import subprocess,tempfile,json,math
ROOT=Path(__file__).resolve().parents[1]
def binary(name):
    p=ROOT/name
    return str(p if p.exists() else p.with_suffix('.exe'))
def run(name,*args,**kw):return subprocess.check_output([binary(name),*map(str,args)],text=True,**kw)
assert run('hash-test').split('Final SHA1: ')[1]==run('hash-serial').split('Final SHA1: ')[1]
parallel=run('integral-test','--table').splitlines()[:-1]
serial=run('integral-serial','--table').splitlines()[:-1]
assert parallel==serial
dx=math.pi/256
for row in parallel:
    m,value=row.split();m=int(m);value=float(value)
    expected=sum(sum(math.sin(x*x)/(1+x*x)+math.exp(-100*x)*math.cos(x*x)/j for j in range(1,m+1))*dx for x in (dx*(i+.5) for i in range(256)))
    assert math.isclose(value,expected,abs_tol=1e-10)
with tempfile.TemporaryDirectory() as directory:
    p=Path(directory);source=p/'input with spaces.txt'
    source.write_text(('123 abc a1\txyz 456 z2\n'*2000)+'789')
    for threads in (1,12):
        run('classifier',source,threads,cwd=p)
        outputs=[[json.loads(line) for line in (p/name).read_text().splitlines()] for name in ['numbers.jsonl','letters.jsonl','mixed.jsonl']]
        assert len(outputs[0])==4001 and len(outputs[1])==4000 and len(outputs[2])==4000
print('serial/parallel hashes, numerical integrals and 12001 token outputs passed')
