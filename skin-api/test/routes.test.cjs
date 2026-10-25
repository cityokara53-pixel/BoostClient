"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const providersRoute = require("../api/providers.js");
const searchRoute = require("../api/skins/search.js");
const skinRoute = require("../api/skins/skin.js");
const catalogRoute = require("../api/skins/catalog.js");
const catalogSkinRoute = require("../api/skins/catalog-skin.js");

function fakeResponse() {
  return {
    headers: {},
    statusCode: 200,
    body: undefined,
    setHeader(name, value) { this.headers[name.toLowerCase()] = value; },
    status(value) { this.statusCode = value; return this; },
    json(value) { this.body = value; return this; },
    end(value) { this.body = value; },
  };
}

test("providers route reports capabilities without exposing environment values", () => {
  const response = fakeResponse();
  providersRoute({ method: "GET", headers: {} }, response);
  assert.equal(response.statusCode, 200);
  assert.equal(response.body.catalog, true);
  assert.equal(response.body.catalogSearch, false);
  assert.ok(Array.isArray(response.body.providers));
  assert.equal(JSON.stringify(response.body).includes("KEY"), false);
});

test("profile lookup route validates inputs before contacting providers", async () => {
  const response = fakeResponse();
  await searchRoute({ method: "GET", headers: {}, query: { username: "invalid name" } }, response);
  assert.equal(response.statusCode, 400);
  assert.match(response.body.error, /valid Minecraft username/i);
});

test("skin route rejects malformed UUIDs before contacting providers", async () => {
  const response = fakeResponse();
  await skinRoute({ method: "GET", headers: {}, query: { uuid: "bad" } }, response);
  assert.equal(response.statusCode, 400);
  assert.match(response.body.error, /UUID/i);
});

test("routes reject methods other than GET", async () => {
  const response = fakeResponse();
  await skinRoute({ method: "POST", headers: {}, query: {} }, response);
  assert.equal(response.statusCode, 405);
  assert.match(response.body.error, /Method not allowed/i);
});

test("API rate limits return a retry hint", () => {
  let lastResponse;
  for (let requestNumber = 0; requestNumber < 61; requestNumber += 1) {
    lastResponse = fakeResponse();
    providersRoute({ method: "GET", headers: { "x-forwarded-for": "route-rate-limit-test" } }, lastResponse);
  }
  assert.equal(lastResponse.statusCode, 429);
  assert.ok(Number(lastResponse.headers["retry-after"]) > 0);
});

test("catalog route returns MineSkin pages without leaking credentials", async () => {
  const service = {
    checkRateLimit: () => ({ allowed: true }),
    listCatalogSkins: async (after) => ({ skins: [{ uuid: "0123456789abcdef0123456789abcdef" }], nextAfter: after || null }),
  };
  const route = catalogRoute.createCatalogRoute(service);
  const response = fakeResponse();
  await route({ method: "GET", headers: {}, query: { after: "0123456789abcdef0123456789abcdef" } }, response);
  assert.equal(response.statusCode, 200);
  assert.deepEqual(response.body, {
    skins: [{ uuid: "0123456789abcdef0123456789abcdef" }],
    nextAfter: "0123456789abcdef0123456789abcdef",
  });
  assert.match(response.headers["cache-control"], /private/);
});

test("catalog-skin route serves only the validated image payload", async () => {
  const service = {
    checkRateLimit: () => ({ allowed: true }),
    downloadCatalogSkin: async () => ({ bytes: Buffer.from([137, 80, 78, 71]), variant: "classic" }),
  };
  const route = catalogSkinRoute.createCatalogSkinRoute(service);
  const response = fakeResponse();
  await route({ method: "GET", headers: {}, query: { uuid: "0123456789abcdef0123456789abcdef" } }, response);
  assert.equal(response.statusCode, 200);
  assert.equal(response.headers["content-type"], "image/png");
  assert.deepEqual(response.body, Buffer.from([137, 80, 78, 71]));
});
