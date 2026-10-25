"use strict";

const { makeSkinService, guardRequest, sendError, sendJson } = require("../../lib/service.cjs");

function createCatalogRoute(service = makeSkinService()) {
  return async (request, response) => {
    if (!guardRequest(request, response, service)) return;
    try {
      const after = typeof request.query?.after === "string" ? request.query.after : undefined;
      if (request.query?.after !== undefined && after === undefined) {
        sendJson(response, 400, { error: "The catalog cursor is invalid." });
        return;
      }
      const catalog = await service.listCatalogSkins(after);
      sendJson(response, 200, catalog, "private, max-age=60, s-maxage=60");
    } catch (error) {
      sendError(response, error);
    }
  };
}

module.exports = createCatalogRoute();
module.exports.createCatalogRoute = createCatalogRoute;
