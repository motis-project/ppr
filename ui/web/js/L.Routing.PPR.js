L.Routing.PPR = L.Class.extend({
  options: {
    serviceUrl: "http://" + window.location.hostname + ":9042/",
    timeout: 30 * 1000,
    searchProfile: defaultSearchProfile,
  },

  initialize: function (options) {
    L.Util.setOptions(this, options);
  },

  route: function (waypoints, callback, context, options) {
    var timedOut = false,
      wps = [],
      url,
      request,
      timer,
      wp,
      i,
      self = this;

    url = this.options.serviceUrl + "route";
    request = this.buildRouteRequest(
      waypoints,
      options,
      self.options.searchProfile
    );
    if (this.options.requestParameters) {
      url += L.Util.getParamString(this.options.requestParameters, url);
    }

    timer = setTimeout(function () {
      timedOut = true;
      callback.call(context || callback, {
        status: -1,
        message: "Routing request timed out.",
      });
    }, this.options.timeout);

    // Create a copy of the waypoints, since they
    // might otherwise be asynchronously modified while
    // the request is being processed.
    for (i = 0; i < waypoints.length; i++) {
      wp = waypoints[i];
      wps.push(new L.Routing.Waypoint(wp.latLng, wp.name, wp.options));
    }

    var headers = new Headers();
    headers.append("Content-Type", "application/json");

    window
      .fetch(url, {
        method: "POST",
        body: JSON.stringify(request),
        headers: headers,
        mode: "cors",
      })
      .then(
        function (response) {
          return response.json();
        },
        function (reason) {
          console.log("routing: fetch failed:", reason);
          callback.call(context || callback, {
            status: -1,
            message: "HTTP request failed: " + reason,
          });
        }
      )
      .then(
        function (response) {
          clearTimeout(timer);
          if (!timedOut) {
            try {
              self._routeDone(response, wps, options, callback, context);
            } catch (ex) {
              callback.call(context || callback, {
                status: -3,
                message: ex.toString(),
              });
            }
          }
        },
        function (reason) {
          console.log("routing: parse failed:", reason);
          callback.call(context || callback, {
            status: -2,
            message: "Error parsing routing response: " + reason,
          });
        }
      );
  },

  _routeDone: function (response, inputWaypoints, options, callback, context) {
    context = context || callback;
    if (response.error !== "") {
      callback.call(context, {
        status: 1,
        message: response.error,
      });
      return;
    }

    var routes = response.routes.map(function (responseRoute) {
      var route = this._convertRoute(responseRoute);
      route.inputWaypoints = inputWaypoints;
      return route;
    }, this);

    callback.call(context, null, routes);
  },

  _convertRoute: function (responseRoute) {
    var coordinates = responseRoute.coordinates.map(function (c) {
      return L.latLng(c[1], c[0]);
    });

    return {
      name: "",
      coordinates: coordinates,
      summary: {
        totalDistance: responseRoute.distance,
        totalTime: responseRoute.duration,
        totalAccessibility: responseRoute.accessibility,
        totalElevationUp: responseRoute.elevation_up,
        totalElevationDown: responseRoute.elevation_down,
      },
      waypoints: [coordinates[0], coordinates[coordinates.length - 1]],
      instructions: responseRoute.steps,
    };
  },

  buildRouteRequest: function (waypoints, options, profile) {
    var locs = [],
      wp,
      latLng,
      endpoint,
      url;

    for (var i = 0; i < waypoints.length; i++) {
      wp = waypoints[i];
      latLng = wp.latLng;
      locs.push(latLng.lng);
      locs.push(latLng.lat);
    }

    return {
      waypoints: locs,
      preview: !!options.geometryOnly,
      profile: profile,
    };
  },
});

L.Routing.ppr = function (options) {
  return new L.Routing.PPR(options);
};
