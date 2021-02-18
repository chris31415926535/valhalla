#include "gurka.h"
#include <gtest/gtest.h>

using namespace valhalla;

TEST(Reachability, break_through_forced_uturn) {
  const std::string ascii_map = R"(
  A----B----C
       1    |
       D    |
       |    |
       E----F)";

  // BD and BE are oneways that lead away, and toward the main road A-B-C
  const gurka::ways ways = {{"AB", {{"highway", "primary"}}}, {"BC", {{"highway", "primary"}}},
                            {"BD", {{"highway", "primary"}}}, {"DE", {{"highway", "primary"}}},
                            {"EF", {{"highway", "primary"}}}, {"CF", {{"highway", "primary"}}}};
  const gurka::nodes nodes = {{"D", {{"barrier", "fence"}, {"access", "no"}}}};
  const auto layout = gurka::detail::map_to_coordinates(ascii_map, 25);

  auto map = gurka::buildtiles(layout, ways, nodes, {}, "test/data/break_through");

  {
    // Check with type="break" for middle waypoint (the default)
    auto result1 = gurka::do_action(valhalla::Options::route, map, {"A", "1", "F"}, "auto");
    gurka::assert::raw::expect_path(result1, {"AB", "BD", "BD", "BC", "CF"});
  }

  {
    // Check with type="break_through" for middle waypoint
    auto result1 = gurka::do_action(valhalla::Options::route, map, {"A", "1", "F"}, "auto",
                                    {{"/locations/1/type", "break_through"}});
    gurka::assert::raw::expect_path(result1, {"AB", "BD", "BD", "BC", "CF"});
  }

  {
    // Force heading due-south for the initial coordinate, which should run the route into the barrier
    // and then do a u-turn
    const std::string& request =
        (boost::format(
             R"({"locations":[{"lat":%s,"lon":%s,"heading":180,"heading_tolerance":45},{"lat":%s,"lon":%s}],"costing":"auto"})") %
         std::to_string(map.nodes.at("1").lat()) % std::to_string(map.nodes.at("1").lng()) %
         std::to_string(map.nodes.at("F").lat()) % std::to_string(map.nodes.at("F").lng()))
            .str();
    auto result1 = gurka::do_action(valhalla::Options::route, map, request);

    gurka::assert::raw::expect_path(result1, {"BD", "BD", "BC", "CF"});
  }
}