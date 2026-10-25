"use strict";

const { makeSkinService, guardRequest, sendError, sendJson } = require("../../lib/service.cjs");

function createCatalogSkinRoute(service = makeSkinService()) {
  return async (request, response) => {
    if (!guardRequest(request, response, service)) return;
    try {
      const uuid = typeof request.query?.uuid === "string" ? request.query.uuid : "";
      if (!uuid) {
        sendJson(response, 400, { error: "A MineSkin skin UUID is required." });
        return;
      }
      const skin = await service.downloadCatalogSkin(uuid);
      response.setHeader("Content-Type", "image/png");
      response.setHeader("Cache-Control", "public, max-age=300, s-maxage=300");
      response.setHeader("X-Content-Type-Options", "nosniff");
      response.setHeader("X-Skin-Variant", skin.variant);
      response.end(skin.bytes);
    } catch (error) {
      sendError(response, error);
    }
  };
}

module.exports = createCatalogSkinRoute();
module.exports.createCatalogSkinRoute = createCatalogSkinRoute;
