const assert = require('node:assert/strict');
const { snapshot, lensLabel } = require('../omarchy-plugin/Model.js');
assert.equal(snapshot('garbage'), null);
assert.equal(snapshot('[]'), null);
assert.equal(snapshot('x'.repeat(17000)), null);
const input = snapshot(JSON.stringify({
  connected: 'true', activeCamera: 1, resolution: 'invalid',
  cameras: [{ id: -1, label: 'bad' }, { id: 1, label: '<img src="https://example.invalid">' }],
}));
assert.equal(input.connected, false);
assert.equal(input.resolution, '1280x720');
assert.equal(input.cameras.length, 1);
assert.equal(lensLabel(input), '<img src="https://example.invalid">');
// Untrusted labels go only through the plugin's PlainText component.
const qml = require('node:fs').readFileSync(require('node:path').join(__dirname, '../omarchy-plugin/Panel.qml'), 'utf8');
assert.match(qml, /component PlainText: Text\s*\{\s*textFormat: Text\.PlainText/);
assert.ok(!qml.includes('tooltipText:'));
console.log('PASS: panel bounds, camera IDs, state validation, and plain-text rendering contract');
