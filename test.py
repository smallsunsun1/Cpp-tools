from example import *
import time

d = Dog()
print(call_go(d))

class Cat(Animal):
    def go(self, n_times):
        return "meow! " * n_times

class Dachshund(Dog):
    def __init__(self, name):
        Dog.__init__(self)
        self.name = name
    def bark(self):
        return "yap!"

c = Cat()
call_go(c)
d = Dachshund("sss")
d.bark()

v1 = Vector2(2, 3)
v2 = Vector2(3, 4)
print(v1 + v2)

vec = IntVector()
print(vec)
# vec.push_back(1)
# vec.push_back(2)
# append_1(vec)
# for ele in vec:
#     print(ele)

dic = MapStringDouble()
dic["ss"] = 1.0
print(dic)


def square(i):
    return i * i
def nowork():
    return

# print(func_arg(square))
# square_plus_1 = func_ret(square)
# print(square_plus_1(4))
# plus_1 = func_cpp()
# print(square_plus_1)
# print(plus_1)

ff = function()
print(ff)
fff = get_func()
print(fff)

func = lambda :print("sss")
print(func)

pool = ThreadPool(4)
res = pool.Submit(fff)
pool.Submit(func)
time.sleep(1)
print(res.valid())