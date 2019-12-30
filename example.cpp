#include <map>
#include <functional>
#include <future>

#include "Eigen/Dense"

#include "pybind11/include/pybind11/pybind11.h"
#include "pybind11/include/pybind11/operators.h"
#include "pybind11/include/pybind11/stl_bind.h"
//#include "pybind11/include/pybind11/stl.h"
#include "pybind11/include/pybind11/functional.h"
#include "libs/stl_thread_pool.h"

namespace py = pybind11;

int add(int i = 1, int j = 2) {
    return i + j;
}

struct Pet {
  explicit Pet(const std::string &name) : name(name) {}
  void SetName(const std::string &name_) { name = name_; }
  const std::string &GetName() const { return name; }
  std::string name;
};
struct Pet2 {
  enum Kind {
    Dog = 0,
    Cat
  };
  Pet2(const std::string &name, Kind type) : name(name), type(type) {}
  void Set(const Pet &pet) { name = pet.name; }
  std::string name;
  Kind type;
};
struct Pet3 {
  Pet3(const std::string &name, int age) : name(name), age(age) {}
  void set(int age_) { age = age_; }
  void set(const std::string &name_) { name = name_; }
  std::string name;
  int age;
};
class Animal {
 public:
  virtual ~Animal() {}
  virtual std::string go(int n_times) = 0;
};
class PyAnimal : public Animal {
 public:
  std::string go(int n_times) override {
      PYBIND11_OVERLOAD_PURE(std::string,
                             Animal, go, n_times);
  }
};
class Dog : public Animal {
 public:
  std::string go(int n_times) override {
      std::string result;
      for (int i = 0; i < n_times; ++i) {
          result += "woof! ";
      }
      return result;
  }
};
std::string call_go(Animal *animal) {
    return animal->go(3);
}

class Vector2 {
 public:
  Vector2(float x, float y) : x(x), y(y) {}
  Vector2 operator+(const Vector2 &v) const { return Vector2(x + v.x, y + v.y); }
  Vector2 operator*(float value) const { return Vector2(x * value, y * value); }
  Vector2 &operator+=(const Vector2 &v) {
      x += v.x;
      y += v.y;
      return *this;
  }
  Vector2 &operator*=(float v) {
      x *= v;
      y *= v;
      return *this;
  }

  friend Vector2 operator*(float f, const Vector2 &v) {
      return Vector2(f * v.x, f * v.y);
  }
  friend Vector2 operator-(const Vector2 &v) {
      return Vector2(-v.x, -v.y);
  }

  std::string toString() const {
      return "[" + std::to_string(x) + ", " + std::to_string(y) + "]";
  }
 private:
  float x;
  float y;
};
class Pickleable {
 public:
  explicit Pickleable(const std::string &value) : m_value(value) {}
  const std::string &value() const { return m_value; }
  void setExtra(int extra) { m_extra = extra; }
  int extra() const { return m_extra; }
 private:
  std::string m_value;
  int m_extra;
};

class Child {};
class Parent {
 public:
  Parent() : child(std::make_shared<Child>()) {}
  std::shared_ptr<Child> get_child() { return child; }
 private:
  std::shared_ptr<Child> child;
};
void append_1(std::vector<int> &v) {
    v.push_back(1);
}
int func_arg(const std::function<int(int)> &f) {
    return f(10);
}
std::function<int(int)> func_ret(const std::function<int(int)> &f) {
    return [f](int i) {
      return f(i) + 1;
    };
}
std::function<void()> func_cpp() {
    return []() {
      std::cout << (100) << std::endl;
    };
}

std::function<void()> get_func() {
    return std::function<void()>([]() {
      std::cout << (100) << std::endl;
    });
}

using Matrix = Eigen::MatrixXd;
using Scalar = Matrix::Scalar;
constexpr bool rowMajor = Matrix::Flags & Eigen::RowMajorBit;

//PYBIND11_MAKE_OPAQUE(std::vector<int>);
//PYBIND11_MAKE_OPAQUE(std::map<std::string, double>);

PYBIND11_MODULE(example, m) {
    m.doc() = "pybind11 example.py plugin"; // optional module docstring
    m.def("add", &add, "A function which adds two numbers");
    m.def("add2", &add, py::arg("i") = 1, py::arg("j") = 2);
    m.attr("the_answer") = 42;
    py::object world = py::cast("World");
    m.attr("what") = world;
    py::class_<Pet>(m, "Pet")
        .def(py::init<const std::string &>())
        .def("SetName", &Pet::SetName)
        .def("GetName", &Pet::GetName)
        .def("__repr__", [](const Pet &a) {
          return "<example.Pet named '" + a.name + "'>";
        });
    py::class_<Pet2> pet2(m, "Pet2");
    pet2.def(py::init<const std::string &, Pet2::Kind>())
        .def("Set", &Pet2::Set)
        .def_readwrite("name", &Pet2::name)
        .def_readwrite("type", &Pet2::type);
    py::enum_<Pet2::Kind>(pet2, "kind")
        .value("Dog", Pet2::Kind::Dog)
        .value("Cat", Pet2::Kind::Cat)
        .export_values();
    py::class_<Pet3>(m, "Pet3")
        .def(py::init<const std::string &, int>())
        .def("set", py::overload_cast<int>(&Pet3::set), "Set the pet's age")
        .def("set", py::overload_cast<const std::string &>(&Pet3::set), "Set the pet's name");
    py::class_<Animal, PyAnimal>(m, "Animal")
        .def(py::init<>())
        .def("go", &Animal::go);
//    py::class_<Animal>(m, "Animal")
//        .def("go", &Animal::go);
    py::class_<Dog, Animal>(m, "Dog")
        .def(py::init<>());
    m.def("call_go", &call_go);
    py::class_<Vector2>(m, "Vector2")
        .def(py::init<float, float>())
        .def(py::self + py::self)
        .def(py::self += py::self)
        .def(py::self *= float())
        .def(float() * py::self)
        .def(py::self * float())
        .def(-py::self)
        .def("__repr__", &Vector2::toString);
    py::class_<Pickleable>(m, "Pickleable")
        .def("value", &Pickleable::value)
        .def("extra", &Pickleable::extra)
        .def("setExtra", &Pickleable::setExtra)
        .def(py::pickle([](const Pickleable &p) {
                          return py::make_tuple(p.value(), p.extra());
                        },
                        [](py::tuple t) {
                          if (t.size() != 2)
                              throw std::runtime_error("Invalid state");
                          Pickleable p(t[0].cast<std::string>());
                          p.setExtra(t[1].cast<int>());
                          return p;
                        }));
    py::class_<Child, std::shared_ptr<Child>>(m, "Child");
    py::class_<Parent, std::shared_ptr<Parent>>(m, "Parent")
        .def(py::init<>())
        .def("get_child", &Parent::get_child);
    py::class_<std::vector<int>>(m, "IntVector")
        .def(py::init<>())
        .def("clear", &std::vector<int>::clear)
        .def("pop_back", &std::vector<int>::pop_back)
        .def("push_back", py::overload_cast<const int &>(&std::vector<int>::push_back), "push element")
        .def("__len__", [](const std::vector<int> &v) { return v.size(); })
        .def("__iter__", [](std::vector<int> &v) {
          return py::make_iterator(v.begin(), v.end());
        }, py::keep_alive<0, 1>());
    m.def("append_1", &append_1);
    py::bind_map<std::map<std::string, double>>(m, "MapStringDouble");
    m.def("func_arg", &func_arg);
    m.def("func_ret", &func_ret);
    m.def("func_cpp", &func_cpp);
    py::class_<std::future<void>>(m, "future_void")
        .def("valid", &std::future<void>::valid)
        .def("get", &std::future<void>::get)
        .def("wait", &std::future<void>::wait);
    py::class_<std::function<void()>>(m, "function")
        .def(py::init<>())
        .def("__call__", &std::function<void()>::operator(), "callable func");
    m.def("get_func", &get_func);
    py::class_<sss::ThreadPool>(m, "ThreadPool")
        .def(py::init<int>())
        .def("Cancel", &sss::ThreadPool::Cancel, "cancel running threadpool")
        .def("Submit",
             (std::future<void> (sss::ThreadPool::*)(std::function<void()>)) &sss::ThreadPool::Submit,
             "submit job");
    py::class_<Matrix>(m, "Matrix", py::buffer_protocol())
        .def("__init__", [](Matrix& m, py::buffer b){
            using Strides = Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>;
            py::buffer_info info = b.request();
          /* Some sanity checks ... */
          if (info.format != py::format_descriptor<Scalar>::format())
              throw std::runtime_error("Incompatible format: expected a double array!");
          if (info.ndim != 2)
              throw std::runtime_error("Incompatible buffer dimension!");
          auto strides = Strides(
              info.strides[rowMajor ? 0 : 1] / (py::ssize_t)sizeof(Scalar),
              info.strides[rowMajor ? 1 : 0] / (py::ssize_t)sizeof(Scalar));
          auto map = Eigen::Map<Matrix, 0, Strides>(static_cast<Scalar*>(info.ptr), info.shape[0], info.shape[1], strides);
          new (&m) Matrix(map);
        })
        .def_buffer([](Matrix& m) -> py::buffer_info {
            return py::buffer_info(
                m.data(),
                sizeof(Scalar),
                py::format_descriptor<Scalar>::format(),
                2,
                {m.rows(), m.cols()},
                {sizeof(Scalar) * (rowMajor ? m.cols() : 1),
                 sizeof(Scalar) * (rowMajor ? 1 : m.rows())}
                );
        });
}
