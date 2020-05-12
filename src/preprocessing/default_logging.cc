#include "ppr/preprocessing/default_logging.h"

namespace ppr::preprocessing {

default_logging::default_logging(logging& log) : log_(log) {
  log_.out_ = &log_stream_;

  log.total_progress_updates_only_ = false;

  log_.step_started_ = [](logging const& log, step_info const& step) {
    std::cout << "===[ " << std::setw(2) << (log.current_step() + 1) << "/"
              << std::setw(2) << log.step_count() << " ]=== " << step.name()
              << std::endl;
  };

  log_.step_progress_ = [](logging const& log, step_info const& step) {
    auto const bar_width = 50;
    auto const progress = step.progress_.progress();
    auto bar_filled = static_cast<int>(progress * bar_width);
    if (bar_filled == bar_width && progress < 1.0) {
      --bar_filled;
    }
    std::cout << "\r   [  " << std::setw(3)
              << static_cast<int>(log.total_progress() * 100) << "% ]   [";
    for (auto i = 0; i < bar_filled; ++i) {
      std::cout << '=';
    }
    if (bar_filled < bar_width) {
      std::cout << '>';
      for (auto i = bar_filled + 1; i < bar_width; ++i) {
        std::cout << ' ';
      }
    }
    std::cout << "]" << std::flush;
  };

  log_.step_finished_ = [this](logging const&, step_info const& step) {
    std::cout << "  " << std::setw(10) << static_cast<int>(step.duration_)
              << "ms" << std::endl;

    auto const log_output = log_stream_.str();
    if (!log_output.empty()) {
      std::clog << log_output << std::endl;
    }
    log_stream_.str(std::string());
    log_stream_.clear();
  };
}

}  // namespace ppr::preprocessing
