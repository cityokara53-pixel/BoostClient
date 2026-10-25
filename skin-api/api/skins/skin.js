"use strict";

const { makeSkinService, normalizeUuid, guardRequest, sendError, sendJson } = require("../../lib/service.cjs");

const service = makeSkinService();

module.exports = async (request, response) => {
  if (!guardRequest(request, response, service)) return;
  const uuid = typeof request.query?.uuid === "string" ? normalizeUuid(request.query.uuid) : null;
  if (!uuid) {
    sendJson(response, 400, { error: "A valid Minecraft UUID is required." });
    return;
  }
  try {
    const { bytes, provider } = await service.downloadSkin(uuid);
    response.statusCode = 200;
    response.setHeader("Content-Type", "image/png");
    response.setHeader("Content-Length", String(bytes.length));
    response.setHeader("Cache-Control", "public, max-age=300, s-maxage=3600");
    response.setHeader("X-Content-Type-Options", "nosniff");
    response.setHeader("X-Skin-Provider", provider);
    response.end(bytes);
  } catch (error) {
    sendError(response, error);
  }
};
