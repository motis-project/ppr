#include "ppr/preprocessing/preprocessing.h"

#include "boost/filesystem.hpp"

#include "ppr/common/timing.h"
#include "ppr/common/verify.h"
#include "ppr/preprocessing/build_routing_graph.h"
#include "ppr/preprocessing/logging.h"
#include "ppr/preprocessing/statistics.h"
#include "ppr/preprocessing/stats_writer.h"
#include "ppr/serialization/reader.h"
#include "ppr/serialization/writer.h"

namespace fs = boost::filesystem;

using namespace ppr::serialization;

namespace ppr::preprocessing {

void write_stats(options const& opt, statistics const& stats) {
  fs::path p = opt.graph_file_;
  p.replace_extension(".stats.csv");
  auto const filename = p.string();
  write_stats(stats, filename);
}

preprocessing_result create_routing_data(options const& opt, logging& log) {
  preprocessing_result result;
  auto& stats = result.stats_;

  if (!boost::filesystem::exists(opt.osm_file_)) {
    log.out() << "File not found: " << opt.osm_file_ << std::endl;
    result.success_ = false;
    result.error_msg_ = "OSM file not found";
    return result;
  }

  {
    auto const t_start = timing_now();
    result.rg_ = build_routing_graph(opt, log, stats);
    auto& rg = result.rg_;
    auto const t_after_build = timing_now();
    stats.d_total_pp_ = ms_between(t_start, t_after_build);

    {
      step_progress progress{log, pp_step::POST_GRAPH_VERIFICATION};
      if (!verify_graph(rg, log.out())) {
        log.out() << "Generated routing graph is invalid!" << std::endl;
        result.success_ = false;
        result.error_msg_ = "Generated routing graph is invalid";
        return result;
      }
    }
    stats.d_verification_ =
        log.get_step_duration(pp_step::POST_GRAPH_VERIFICATION);

    {
      step_progress progress{log, pp_step::POST_SERIALIZATION};
      rg.filename_ = opt.graph_file_;
      write_routing_graph(rg, opt.graph_file_, stats);
    }
    stats.d_serialization_ = log.get_step_duration(pp_step::POST_SERIALIZATION);

    if (opt.create_rtrees_) {
      step_progress progress{log, pp_step::POST_RTREES};
      fs::remove(fs::path(rg.filename_ + ".ert"));
      fs::remove(fs::path(rg.filename_ + ".art"));
      rg.prepare_for_routing(opt.edge_rtree_max_size_,
                             opt.area_rtree_max_size_);
    }
    stats.d_rtrees_ = log.get_step_duration(pp_step::POST_RTREES);

    auto const t_end = timing_now();
    stats.d_total_ = ms_between(t_start, t_end);

    write_stats(opt, stats);
  }

  return result;
}

}  // namespace ppr::preprocessing
