//
// Created by James Sherman on 9/6/19.
//

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "sanity_check", "Prove that 1 equals 1" ){
int one = 1;
REQUIRE( one == 1 );
}