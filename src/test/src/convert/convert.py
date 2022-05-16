import sys
import re

outStr = '{{ std::make_tuple("{value}", "{type}", "{sqltype}"), "{correct}" }},'
# print(outStr.format(value="Hello", type="varchar", sqltype="idk", correct=""))

# match 'true' (boolean) as SQL_C_SBIGINT: 1
matchStr = "^'(.*)' \((\w+)\) as ([A-Z_]+):( (.+))?$"
regex = re.compile(matchStr)

for line in sys.stdin:
    # pull out the data
    match = regex.match(line)
    if match is None:
        print("Did not match line: {line}".format(line=line), file=sys.stderr)
        continue
    
    # format it correctly
    fmtedStr = outStr.format(
        value=match.group(1),
        type=match.group(2),
        sqltype=match.group(3),
        correct=match.group(5)
    )

    # print it out
    print(fmtedStr)
