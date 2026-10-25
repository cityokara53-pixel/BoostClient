"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const { makeSkinService, MAX_RESPONSE_BYTES } = require("../lib/service.cjs");

const UUID = "0123456789abcdef0123456789abcdef";
const PNG = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10, 0]);

function responseJson(value, status = 200) {
  return new Response(JSON.stringify(value), {
    status,
    headers: { "content-type": "application/json" },
  });
}

function profileResponse() {
  const texture = Buffer.from(JSON.stringify({
    textures: { SKIN: { url: "https://textures.minecraft.net/texture/0123456789abcdef0123456789abcdef" } },
  })).toString("base64");
  return responseJson({
    id: UUID,
    name: "TestUser",
    properties: [{ name: "textures", value: texture }],
  });
}

test("rejects invalid UUIDs before making provider requests", async () => {
  let calls = 0;
  const service = makeSkinService({ fetchImpl: async () => { calls += 1; } });
  await assert.rejects(service.getProfileByUuid("not-a-uuid"), { status: 400 });
  assert.equal(calls, 0);
});

test("does not call the profile service when profile lookup is disabled", async () => {
  let calls = 0;
  const service = makeSkinService({
    env: { SKIN_API_MOJANG_PROFILE_ENABLED: "false" },
    fetchImpl: async () => { calls += 1; },
  });
  await assert.rejects(service.getProfileByUuid(UUID), { status: 503 });
  assert.equal(calls, 0);
});

test("downloads a UUID skin from Crafatar without requiring Mojang profile lookup", async () => {
  const urls = [];
  const service = makeSkinService({
    env: { SKIN_API_PROVIDER_ORDER: "crafatar", SKIN_API_MOJANG_TEXTURES_ENABLED: "false" },
    fetchImpl: async (url) => {
      urls.push(String(url));
      return new Response(PNG, { headers: { "content-type": "image/png" } });
    },
  });
  const result = await service.downloadSkin(UUID);
  assert.equal(result.provider, "crafatar");
  assert.deepEqual(result.bytes, PNG);
  assert.deepEqual(urls, [`https://crafatar.com/skins/${UUID}`]);
});

test("falls back to Crafatar when Mojang texture download fails", async () => {
  const urls = [];
  const service = makeSkinService({
    fetchImpl: async (url) => {
      urls.push(String(url));
      if (String(url).startsWith("https://sessionserver.mojang.com/")) return profileResponse();
      if (String(url).startsWith("https://textures.minecraft.net/")) return responseJson({}, 502);
      return new Response(PNG, { headers: { "content-type": "image/png" } });
    },
  });
  const result = await service.downloadSkin(UUID);
  assert.equal(result.provider, "crafatar");
  assert.equal(urls.at(-1), `https://crafatar.com/skins/${UUID}`);
});

test("fails safely for invalid provider JSON", async () => {
  const service = makeSkinService({
    fetchImpl: async () => new Response("{invalid", { headers: { "content-type": "application/json" } }),
  });
  await assert.rejects(service.getProfileByUuid(UUID), /invalid JSON/i);
});

test("rejects non-PNG and malformed image responses", async () => {
  const service = makeSkinService({
    env: { SKIN_API_PROVIDER_ORDER: "crafatar", SKIN_API_MOJANG_TEXTURES_ENABLED: "false" },
    fetchImpl: async () => new Response("not a PNG", { headers: { "content-type": "image/png" } }),
  });
  await assert.rejects(service.downloadSkin(UUID), { status: 502 });
});

test("aborts a provider request when its configured timeout expires", async () => {
  const service = makeSkinService({
    env: { SKIN_API_TIMEOUT_MS: "500" },
    fetchImpl: (_url, { signal }) => new Promise((_resolve, reject) => {
      signal.addEventListener("abort", () => reject(new DOMException("Aborted", "AbortError")), { once: true });
    }),
  });
  await assert.rejects(service.getProfileByUuid(UUID), { name: "AbortError" });
});

test("rejects oversized provider bodies", async () => {
  const service = makeSkinService({
    fetchImpl: async () => new Response(new Uint8Array(MAX_RESPONSE_BYTES + 1)),
  });
  await assert.rejects(service.getProfileByUuid(UUID), /size limit/i);
});

test("returns a clear unavailable error when every image provider is disabled", async () => {
  let calls = 0;
  const service = makeSkinService({
    env: {
      SKIN_API_MOJANG_TEXTURES_ENABLED: "false",
      SKIN_API_CRAFATAR_ENABLED: "false",
    },
    fetchImpl: async () => { calls += 1; },
  });
  await assert.rejects(service.downloadSkin(UUID), { status: 503 });
  assert.equal(calls, 0);
});

test("MineSkin catalog requests keep the API key server-side and validate pagination", async () => {
  const requests = [];
  const service = makeSkinService({
    env: { MINESKIN_API_KEY: "server-only-test-key" },
    fetchImpl: async (url, options) => {
      requests.push({ url: String(url), headers: options.headers });
      return responseJson({
        skins: [
          { uuid: UUID, name: "Test Skin", visibility: "public", variant: "slim", views: 7 },
          { uuid: "fedcba9876543210fedcba9876543210", name: "Private", visibility: "private", variant: "classic", views: 1 },
          { uuid: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", shortId: "aaaaaaaa", name: null },
        ],
        links: { next: `/v2/skins?after=${UUID}` },
      });
    },
  });
  const page = await service.listCatalogSkins();
  assert.equal(requests[0].url, "https://api.mineskin.org/v2/skins");
  assert.equal(requests[0].headers.Authorization, "Bearer server-only-test-key");
  assert.match(requests[0].headers["User-Agent"], /^BoostClient\//);
  assert.deepEqual(page, {
    skins: [
      { uuid: UUID, name: "Test Skin", variant: "slim", visibility: "public", views: 7 },
      { uuid: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", name: "", variant: "unknown", visibility: "public", views: 0 },
    ],
    nextAfter: UUID,
  });
  await assert.rejects(service.listCatalogSkins("not-a-uuid"), { status: 400 });
  assert.equal(requests.length, 1);
});

test("MineSkin catalog requires its secret and retrieves PNGs only from the texture host", async () => {
  let calls = 0;
  const missingKey = makeSkinService({ env: {}, fetchImpl: async () => { calls += 1; } });
  await assert.rejects(missingKey.listCatalogSkins(), { status: 503 });
  assert.equal(calls, 0);

  const requests = [];
  const service = makeSkinService({
    env: { MINESKIN_API_KEY: "server-only-test-key" },
    fetchImpl: async (url, options) => {
      requests.push({ url: String(url), headers: options.headers });
      if (String(url) === `https://api.mineskin.org/v2/skins/${UUID}`) {
        return responseJson({
          skin: {
            uuid: UUID,
            visibility: "public",
            variant: "slim",
            texture: { url: { skin: "https://textures.minecraft.net/texture/0123456789abcdef0123456789abcdef" } },
          },
        });
      }
      return new Response(PNG, { headers: { "content-type": "image/png" } });
    },
  });
  const result = await service.downloadCatalogSkin(UUID);
  assert.deepEqual(result, { bytes: PNG, variant: "slim" });
  assert.equal(requests[0].headers.Authorization, "Bearer server-only-test-key");
  assert.equal(requests[1].url, "https://textures.minecraft.net/texture/0123456789abcdef0123456789abcdef");
  assert.equal(Object.values(requests[1].headers).includes("Bearer server-only-test-key"), false);
});

test("MineSkin catalog rejects texture URLs outside the fixed Minecraft host", async () => {
  const service = makeSkinService({
    env: { MINESKIN_API_KEY: "server-only-test-key" },
    fetchImpl: async () => responseJson({
      skin: {
        uuid: UUID,
        visibility: "public",
        variant: "classic",
        texture: { url: { skin: "https://example.com/skin.png" } },
      },
    }),
  });
  await assert.rejects(service.downloadCatalogSkin(UUID), /unsupported texture URL/i);
});

test("enforces the per-process request rate limit", () => {
  let now = 0;
  const service = makeSkinService({ now: () => now });
  for (let index = 0; index < 60; index += 1) {
    assert.equal(service.checkRateLimit("client").allowed, true);
  }
  const limited = service.checkRateLimit("client");
  assert.equal(limited.allowed, false);
  assert.ok(limited.retryAfter > 0);
  now = 60_001;
  assert.equal(service.checkRateLimit("client").allowed, true);
});
