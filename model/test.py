import helloworld
from helloworld import _JObject, _JInt, _JException, _JFloat, _JChar
print(dir(helloworld))

if True:
    print("******************")
    class U(_JObject):
        pass

    class V(_JObject):
        pass

    u = U()

    # Lets try to use a concrete type 
    #class V(type(u)):
    #    pass


    print("******************")

    print("MRO", U.__mro__)
    print("alloc")
    print(repr(U))
    print(repr(type(u)))
    print("MRO", type(u).__mro__)

    u.__set_slot__(42)
    #setattr(u, "hello", 1)
    #print(u.hello)
    print("U slot", u.__get_slot__())


if False:
    # Can we get/set on TObject?
    o = _JObject()
    o.__set_slot__(42)
    print("O slot", o.__get_slot__())
    pass

if False:
    i=_JInt(0)
    i=_JInt(2**15-1)
    i=_JInt(2**31-1)
    i=_JInt(2**63-1)
    i.__set_slot__(42)
    print(i)
    print("I slot", i.__get_slot__())

print("=====")
e = _JException()
e.__set_slot__(42)
print(e.__get_slot__())


print("=====")
d=_JFloat(1.45)
d.__set_slot__(42)
print(d.__get_slot__())
print(d)

print("=====")
s=_JChar("hello")
s.__set_slot__(42)
print(s.__get_slot__())
print(s)

print("done")

