import json

already_seen = set()

def find_order_by_index(i, deep):
    already_seen.add(i)
    result = ('\t' * deep)
    result += manifest["types"][i]["name"]
    result += '\n'
    for ii in range(len(manifest["types"])):
        if ii not in already_seen and manifest["types"][ii]["parentIndex"] == i:
            result += find_order_by_index(ii, deep + 1)
    return result

def find_order_by_type(type):
    result = ""
    while True:
        result += type["name"]
        result += " -> "
        if(type["parentIndex"] == -1):
            break
        type = manifest["types"][type["parentIndex"]]
    result += '\n'
    return result


with open("C:\\Users\\Administrator\\Desktop\\logexts\\Debug\\manifest.json") as f:
    manifest = json.load(f)
with open("1827.txt", "w") as f:
    for i in range(len(manifest["types"])):
        if i not in already_seen:
            f.write(find_order_by_index(i, 0))
    # for type in manifest["types"]:
    #     f.write(find_order(type))