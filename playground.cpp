//
// Created by wuyuanyi on 2019-09-24.
//
#include <xtensor/xarray.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xmath.hpp>
#include <rx.hpp>
#include <iostream>
#include <fmt/format.h>

int main() {
    xt::xarray<double> a{{1,2,3,4,5} };
    a.resize({2,2});
    a.fill(1);
    a(0,2) = 2;
    a(0,0) = 9;
    std::cout << a << std::endl;

    auto prod = xt::prod(a);
    std::cout<< prod << std::endl;

    std::cout <<a(0) << std::endl;
    std::cout <<a(1) << std::endl;
    std::cout <<a(2) << std::endl;
    std::cout <<a(3) << std::endl;

    // rx copy or ref?
    struct abc {
      int a;
      float b;
    };
    rxcpp::subjects::subject<abc> sub;
    auto subscriber = rxcpp::make_subscriber<abc>([](abc o){std::cout<<"after1 : "<<&o<<std::endl;});
    auto subscriber2 = rxcpp::make_subscriber<abc>([](abc o){std::cout<<"after2 : "<<&o<<std::endl;});
    sub.get_observable().subscribe(subscriber);
    sub.get_observable().subscribe(subscriber2);
    abc original;
    std::cout << "original: " << &original <<std::endl;
    sub.get_subscriber().on_next(original);


    // fmt
    std::cout <<fmt::format("Hello, {}", "World") << std::endl;

    // no element?
    auto aaa = xt::xarray<double>();
    std::cout <<aaa.size() << std::endl;
    return 0;
}