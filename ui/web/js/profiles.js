var defaultSearchProfile = {
  walking_speed: 1.4,
  duration_limit: 60 * 60,
  max_crossing_detour_primary: 300,
  max_crossing_detour_secondary: 200,
  max_crossing_detour_tertiary: 200,
  max_crossing_detour_residential: 100,
  max_crossing_detour_service: 0,
  round_distance: 0,
  round_duration: 30,
  round_accessibility: 5,
  max_routes: 0,
  divisions_duration: 0,
  divisions_accessibility: 0,
  crossing_primary: {
    signals: { duration: [120], accessibility: [], allowed: "allowed" },
    marked: { duration: [100], accessibility: [], allowed: "allowed" },
    island: { duration: [200], accessibility: [], allowed: "allowed" },
    unmarked: {
      duration: [100],
      accessibility: [],
      allowed: "penalized",
      duration_penalty: 200,
      accessibility_penalty: 0
    }
  },
  crossing_secondary: {
    signals: { duration: [60], accessibility: [], allowed: "allowed" },
    marked: { duration: [30], accessibility: [], allowed: "allowed" },
    island: { duration: [60], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [100], accessibility: [], allowed: "allowed" }
  },
  crossing_tertiary: {
    signals: { duration: [60], accessibility: [], allowed: "allowed" },
    marked: { duration: [30], accessibility: [], allowed: "allowed" },
    island: { duration: [60], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [100], accessibility: [], allowed: "allowed" }
  },
  crossing_residential: {
    signals: { duration: [45], accessibility: [], allowed: "allowed" },
    marked: { duration: [20], accessibility: [], allowed: "allowed" },
    island: { duration: [40], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [30], accessibility: [], allowed: "allowed" }
  },
  crossing_service: {
    signals: { duration: [], accessibility: [], allowed: "allowed" },
    marked: { duration: [], accessibility: [], allowed: "allowed" },
    island: { duration: [], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [], accessibility: [], allowed: "allowed" }
  },
  crossing_rail: { duration: [60], accessibility: [], allowed: "allowed" },
  crossing_tram: { duration: [30], accessibility: [], allowed: "allowed" },
  stairs_up_cost: { duration: [], accessibility: [0, 0], allowed: "allowed" },
  stairs_down_cost: { duration: [], accessibility: [0, 0], allowed: "allowed" },
  stairs_with_handrail_up_cost: {
    duration: [],
    accessibility: [0, 0],
    allowed: "allowed"
  },
  stairs_with_handrail_down_cost: {
    duration: [],
    accessibility: [0, 0],
    allowed: "allowed"
  },
  elevator_cost: { duration: [60], accessibility: [], allowed: "allowed" },
  escalator_cost: { duration: [], accessibility: [0], allowed: "allowed" },
  moving_walkway_cost: { duration: [], accessibility: [], allowed: "allowed" },
  elevation_up_cost: { duration: [], accessibility: [], allowed: "allowed" },
  elevation_down_cost: { duration: [], accessibility: [], allowed: "allowed" }
};

var accessibility1Profile = Object.assign({}, defaultSearchProfile, {
  crossing_primary: Object.assign({}, defaultSearchProfile.crossing_primary, {
    unmarked: {
      duration: [100],
      accessibility: [],
      allowed: "penalized",
      duration_penalty: 200,
      accessibility_penalty: 0
    }
  }),
  crossing_secondary: Object.assign(
    {},
    defaultSearchProfile.crossing_secondary,
    {
      unmarked: {
        duration: [100],
        accessibility: [],
        allowed: "penalized",
        duration_penalty: 200,
        accessibility_penalty: 0
      }
    }
  ),
  crossing_tertiary: Object.assign(
    {},
    defaultSearchProfile.crossing_secondary,
    {
      unmarked: {
        duration: [100],
        accessibility: [],
        allowed: "penalized",
        duration_penalty: 200,
        accessibility_penalty: 0
      }
    }
  ),
  crossing_rail: { duration: [60], accessibility: [10], allowed: "allowed" },
  crossing_tram: { duration: [30], accessibility: [5], allowed: "allowed" },
  stairs_up_cost: { duration: [], accessibility: [10, 2], allowed: "allowed" },
  stairs_down_cost: {
    duration: [],
    accessibility: [10, 1],
    allowed: "allowed"
  },
  stairs_with_handrail_up_cost: {
    duration: [],
    accessibility: [8, 2],
    allowed: "allowed"
  },
  stairs_with_handrail_down_cost: {
    duration: [],
    accessibility: [8, 1],
    allowed: "allowed"
  },
  elevation_up_cost: { duration: [], accessibility: [0, 1], allowed: "allowed" }
});

var accessibility2Profile = Object.assign({}, accessibility1Profile, {
  stairs_up_cost: {
    duration: [],
    accessibility: [10, 2],
    allowed: "forbidden"
  },
  stairs_down_cost: {
    duration: [],
    accessibility: [10, 1],
    allowed: "forbidden"
  },
  stairs_with_handrail_up_cost: {
    duration: [],
    accessibility: [8, 2],
    allowed: "forbidden"
  },
  stairs_with_handrail_down_cost: {
    duration: [],
    accessibility: [8, 1],
    allowed: "forbidden"
  },
  escalator_cost: { duration: [], accessibility: [0], allowed: "forbidden" },
  elevation_up_cost: {
    duration: [],
    accessibility: [0, 3],
    allowed: "allowed"
  },
  elevation_down_cost: {
    duration: [],
    accessibility: [0, 1],
    allowed: "allowed"
  }
});

var accessibility2Max3Profile = Object.assign({}, accessibility2Profile, {
  max_routes: 3,
  divisions_duration: 3,
  divisions_accessibility: 3
});

var accessibility2Max5Profile = Object.assign({}, accessibility2Profile, {
  max_routes: 5,
  divisions_duration: 5,
  divisions_accessibility: 5
});

var elevationProfile = Object.assign({}, defaultSearchProfile, {
  elevation_up_cost: {
    duration: [],
    accessibility: [0, 2],
    allowed: "allowed"
  },
  elevation_down_cost: {
    duration: [],
    accessibility: [0, 1],
    allowed: "allowed"
  }
});

var noCrossingDetoursProfile = Object.assign({}, defaultSearchProfile, {
  max_crossing_detour_primary: 0,
  max_crossing_detour_secondary: 0,
  max_crossing_detour_tertiary: 0,
  max_crossing_detour_residential: 0
});

var simpleProfile = Object.assign({}, noCrossingDetoursProfile, {
  crossing_primary: {
    signals: { duration: [0], accessibility: [], allowed: "allowed" },
    marked: { duration: [0], accessibility: [], allowed: "allowed" },
    island: { duration: [0], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [0], accessibility: [], allowed: "allowed" }
  },
  crossing_secondary: {
    signals: { duration: [0], accessibility: [], allowed: "allowed" },
    marked: { duration: [0], accessibility: [], allowed: "allowed" },
    island: { duration: [0], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [0], accessibility: [], allowed: "allowed" }
  },
  crossing_tertiary: {
    signals: { duration: [0], accessibility: [], allowed: "allowed" },
    marked: { duration: [0], accessibility: [], allowed: "allowed" },
    island: { duration: [0], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [0], accessibility: [], allowed: "allowed" }
  },
  crossing_residential: {
    signals: { duration: [0], accessibility: [], allowed: "allowed" },
    marked: { duration: [0], accessibility: [], allowed: "allowed" },
    island: { duration: [0], accessibility: [], allowed: "allowed" },
    unmarked: { duration: [0], accessibility: [], allowed: "allowed" }
  },
  crossing_rail: { duration: [0], accessibility: [], allowed: "allowed" },
  crossing_tram: { duration: [0], accessibility: [], allowed: "allowed" },
  elevator_cost: { duration: [0], accessibility: [], allowed: "allowed" }
});

var searchProfiles = [
  { profile: defaultSearchProfile, name: "Standard" },
  { profile: accessibility1Profile, name: "Auch nach leichten Wegen suchen" },
  { profile: accessibility2Profile, name: "Rollstuhl" },
  { profile: elevationProfile, name: "Weniger Steigung" },
  { profile: noCrossingDetoursProfile, name: "Ampeln nicht bevorzugen" },
  { profile: simpleProfile, name: "Simpel" }
];
