"use strict";

const { makeSkinService, guardRequest, sendJson } = require("../lib/service.cjs");

const service = makeSkinService();

module.exports = (request, response) => {
  if (!guardRequest(request, response, service)) return;
  sendJson(response, 200, {
    providers: service.listProviders(),
    catalog: true,
    catalogConfigured: Boolean(process.env.MINESKIN_API_KEY),
    catalogSearch: false,
    catalogProvider: "MineSkin",
    note: "The MineSkin catalog provides a paginated public skin library. Search filters the currently loaded page in the launcher.",
  }, "public, max-age=300, s-maxage=300");
};
