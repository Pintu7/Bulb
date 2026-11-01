// Builds res/Bulb*.ico from bulb.svg (24px and up) and bulb-16.svg (16px), using headless Chrome to rasterize.
// Usage: node res/icon-src/build-icons.mjs [previewDir]
import { readFileSync, writeFileSync, mkdtempSync, existsSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const resDir = join(here, '..');
const previewDir = process.argv[2];

// Tray states: same shape, the bulb glass and base change colour.
const states = {
  'Bulb.ico': null,
  'Bulb_NoCopyCb.ico': { '#EF9F27': '#B4B2A9', '#FAC775': '#D3D1C7' }, // copying paused: bulb off
  'Bulb_NoDb.ico': { '#EF9F27': '#E24B4A', '#FAC775': '#F09595' },     // no database: bulb red
};
const sizes = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256];

const chrome = [
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
].find(existsSync);
if (!chrome) throw new Error('Chrome or Edge not found');

const big = readFileSync(join(here, 'bulb.svg'), 'utf8');
const small = readFileSync(join(here, 'bulb-16.svg'), 'utf8');
const recolor = (svg, map) => map ? svg.replace(/#[0-9A-F]{6}/gi, c => map[c.toUpperCase()] ?? c) : svg;
const sized = (svg, s) => svg.replace(/width="\d+" height="\d+"/, `width="${s}" height="${s}"`);

// One job per (file, size): the page draws each SVG into a canvas and dumps RGBA + PNG as JSON.
const jobs = [];
for (const [file, map] of Object.entries(states))
  for (const s of sizes)
    jobs.push({ file, s, svg: sized(recolor(s === 16 ? small : big, map), s) });

const html = `<!doctype html><pre id="out"></pre><script>
const jobs = ${JSON.stringify(jobs.map(j => ({ s: j.s, url: 'data:image/svg+xml;base64,' + Buffer.from(j.svg).toString('base64') })))};
Promise.all(jobs.map(j => new Promise(ok => {
  const img = new Image();
  img.onload = () => {
    const c = document.createElement('canvas'); c.width = c.height = j.s;
    const g = c.getContext('2d'); g.drawImage(img, 0, 0, j.s, j.s);
    const d = g.getImageData(0, 0, j.s, j.s).data;
    let bin = ''; for (let i = 0; i < d.length; i++) bin += String.fromCharCode(d[i]);
    ok({ rgba: btoa(bin), png: c.toDataURL('image/png').split(',')[1] });
  };
  img.src = j.url;
}))).then(r => { document.getElementById('out').textContent = JSON.stringify(r); });
</script>`;

const tmp = mkdtempSync(join(tmpdir(), 'bulb-icons-'));
const page = join(tmp, 'render.html');
writeFileSync(page, html);
const dom = execFileSync(chrome, [
  '--headless=new', '--disable-gpu', `--user-data-dir=${join(tmp, 'profile')}`,
  '--virtual-time-budget=10000', '--dump-dom', pathToFileURL(page).href,
], { maxBuffer: 256 << 20 }).toString();
const json = dom.match(/<pre id="out">([\s\S]*?)<\/pre>/)[1].replace(/&quot;/g, '"');
const results = JSON.parse(json);
if (results.length !== jobs.length) throw new Error('render incomplete');

// 32bpp BMP entry: header with doubled height, bottom-up BGRA rows, empty AND mask.
function bmp(s, rgba) {
  const maskRow = Math.ceil(s / 32) * 4;
  const buf = Buffer.alloc(40 + s * s * 4 + maskRow * s);
  buf.writeUInt32LE(40, 0); buf.writeInt32LE(s, 4); buf.writeInt32LE(s * 2, 8);
  buf.writeUInt16LE(1, 12); buf.writeUInt16LE(32, 14);
  for (let y = 0; y < s; y++)
    for (let x = 0; x < s; x++) {
      const src = (y * s + x) * 4, dst = 40 + ((s - 1 - y) * s + x) * 4;
      buf[dst] = rgba[src + 2]; buf[dst + 1] = rgba[src + 1]; buf[dst + 2] = rgba[src]; buf[dst + 3] = rgba[src + 3];
    }
  return buf;
}

for (const file of Object.keys(states)) {
  const entries = jobs.map((j, i) => ({ ...j, r: results[i] })).filter(j => j.file === file);
  const images = entries.map(e => e.s === 256 ? Buffer.from(e.r.png, 'base64') : bmp(e.s, Buffer.from(e.r.rgba, 'base64')));
  const head = Buffer.alloc(6 + 16 * images.length);
  head.writeUInt16LE(1, 2); head.writeUInt16LE(images.length, 4);
  let offset = head.length;
  entries.forEach((e, i) => {
    const o = 6 + 16 * i;
    head[o] = e.s % 256; head[o + 1] = e.s % 256;
    head.writeUInt16LE(1, o + 4); head.writeUInt16LE(32, o + 6);
    head.writeUInt32LE(images[i].length, o + 8); head.writeUInt32LE(offset, o + 12);
    offset += images[i].length;
  });
  writeFileSync(join(resDir, file), Buffer.concat([head, ...images]));
  if (previewDir) entries.forEach(e => writeFileSync(join(previewDir, `${file.replace('.ico', '')}-${e.s}.png`), Buffer.from(e.r.png, 'base64')));
  console.log(file, entries.map(e => e.s).join(' '));
}
