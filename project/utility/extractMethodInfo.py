import glob
import re

methods = {}
for i in glob.iglob("**/*.java", recursive=True):
    with open(i, 'r') as fd:
        lines = fd.readlines()
    for l in lines:
        if l.strip().startswith("//"):
            continue
        if not "@PyMethodInfo" in l:
            continue

        d = {}
        for q in l.split("(")[1].split(")")[0].split(","):
            m = re.search(r'name = "(.*)"', q)
            if m is not None:
                d['name'] = m.group(1)
            m = re.search(r'invoke = (.*)', q)
            if m is not None:
                d['invoke'] = m.group(1)
        methods[d['name']] = d

for m in sorted(methods.keys()):
    u = methods[m]
    print('REGISTER_CALL("%s", %s); // %s' % (u['name'], u['name'], u['invoke']))
