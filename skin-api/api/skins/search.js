"use strict";

const { makeSkinService, guardRequest, sendError, sendJson } = require("../../lib/service.cjs");

const service = makeSkinService();

module.exports = async (request, response) => {
  if (!guardRequest(request, response, service)) return;
  try {
    const username = typeof request.query?.username === "string" ? request.query.username : "";
    const uuid = typeof request.query?.uuid === "string" ? request.query.uuid : "";
    if (Boolean(username) === Boolean(uuid)) {
      sendJson(response, 400, { error: "Provide exactly one username or UUID." });
      return;
    }
    const profile = username
      ? await service.lookupUsername(username)
      : await service.getProfileByUuid(uuid);
    sendJson(response, 200, { skins: [profile] }, "public, max-age=120, s-maxage=300");
  } catch (error) {
    sendError(response, error);
  }
};
