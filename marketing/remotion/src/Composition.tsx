import React from 'react';
import {AbsoluteFill, CanvasImage, Easing, interpolate, staticFile, useCurrentFrame, useVideoConfig} from 'remotion';
import {Audio} from '@remotion/media';
import {loadFont} from '@remotion/fonts';
import doctor from '../public/research/doctor.json';

loadFont({family: 'JetBrains', url: staticFile('fonts/JetBrainsMono-Regular.ttf'), weight: '400'});
loadFont({family: 'JetBrains', url: staticFile('fonts/JetBrainsMono-Bold.ttf'), weight: '700'});

const C = {bg: '#0c0b0c', ink: '#fafcfb', dim: '#aaa6a4', border: '#595655', green: '#a4b9a6'};
const clamp = {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'} as const;
const smooth = Easing.bezier(.65, 0, .15, 1);
const PANEL = {x: 180, y: 170, w: 768, h: 816};
const TERMINAL = {x: 1375, y: 250, w: 1160, h: 720};

// Repeated values produce explicit holds. Every layer uses this one world.
const T = [0, 30, 54, 106, 148, 232, 270, 304, 344, 449];
const X = [565, 565, 650, 650, 1945, 1945, 1335, 1335, 1710, 1710];
const Y = [380, 380, 305, 305, 570, 570, 560, 560, 1780, 1780];
const Z = [1.35, 1.35, 1.5, 1.5, 1.05, 1.05, .55, .55, 1.04, 1.04];

const SourcePanel: React.FC = () => {
  const f = useCurrentFrame();
  const on = f >= 76;
  const toggle = interpolate(f, [76, 83], [0, 1], {...clamp, easing: smooth});
  return <div style={{position: 'absolute', left: PANEL.x, top: PANEL.y, width: PANEL.w, height: PANEL.h}}>
    <CanvasImage src={staticFile('research/panel-off.png')} style={{width: '100%', height: '100%'}} />
    {/* Only existing control/meta pixels change; source PNG supplies the full real UI. */}
    {on && <>
      <div style={{position: 'absolute', left: 112, top: 77, width: 475, height: 36, background: C.bg, fontSize: 22, fontWeight: 700, color: C.dim}}>USB · CAMERA ON</div>
      <div style={{position: 'absolute', left: 648, top: 55, width: 70, height: 39, background: C.bg}}>
        <div style={{position: 'absolute', left: 2, top: 1, width: 65, height: 35, border: '1.5px solid #92918f', background: '#353433'}} />
        <div style={{position: 'absolute', left: 6 + toggle * 30, top: 6, width: 24, height: 25, background: C.ink}} />
      </div>
    </>}
  </div>;
};

const Pointer: React.FC = () => {
  const f = useCurrentFrame();
  const x = interpolate(f, [0, 25, 54, 72, 82, 106, 148, 205, 230], [590, 850, 865, 865, 865, 865, 1454, 1650, 1650], {...clamp, easing: smooth});
  const y = interpolate(f, [0, 25, 54, 72, 82, 106, 148, 205, 230], [710, 285, 250, 250, 250, 250, 350, 350, 350], {...clamp, easing: smooth});
  const press = interpolate(f, [73, 76, 80, 83], [1, .85, .85, 1], {...clamp});
  return <div style={{position: 'absolute', left: x, top: y, opacity: f > 232 ? 0 : 1, scale: String(press), transformOrigin: '0 0'}}>
    <svg width="38" height="49" viewBox="0 0 38 49"><path d="M2 2V37L12 27L21 46L29 42L20 24H35Z" fill={C.ink} stroke={C.bg} strokeWidth="2.8" /></svg>
  </div>;
};

const Terminal: React.FC = () => {
  const f = useCurrentFrame();
  const command = '~ $ iphonecam doctor --json';
  const characters = Math.floor(interpolate(f, [153, 180], [0, command.length], clamp));
  const result = f >= 190;
  return <div style={{position: 'absolute', left: TERMINAL.x, top: TERMINAL.y, width: TERMINAL.w, height: TERMINAL.h, border: `2px solid ${C.border}`, background: '#101113'}}>
    <div style={{height: 54, borderBottom: '1px solid #343436', padding: '12px 30px', fontSize: 22, color: C.dim}}>iphonecam · USB camera output</div>
    <div style={{padding: '32px 34px', fontSize: 42, lineHeight: 1.5, whiteSpace: 'pre', color: C.ink}}>
      <div>{command.slice(0, characters)}<span style={{opacity: f < 185 && Math.floor(f / 9) % 2 === 0 ? 1 : 0}}>▊</span></div>
      {result && <div style={{marginTop: 30, fontSize: 42}}>
        <div style={{color: C.dim}}>{'{'}</div>
        <div>  <span style={{color: C.dim}}>"device":</span> "{doctor.device}",</div>
        <div>  <span style={{color: C.dim}}>"virtualCamera":</span> <span style={{color: C.green}}>{String(doctor.virtualCamera)}</span>,</div>
        <div>  <span style={{color: C.dim}}>"ok":</span> <span style={{color: C.green}}>{String(doctor.ok)}</span></div>
        <div style={{color: C.dim}}>{'}'}</div>
      </div>}
    </div>
    <div style={{position: 'absolute', left: 34, bottom: 35, fontSize: 28, color: C.dim}}>iPhone Camera <span style={{color: C.ink}}>→</span> /dev/video42</div>
  </div>;
};

const Connection: React.FC = () => {
  const f = useCurrentFrame();
  const progress = interpolate(f, [100, 148], [0, 1], {...clamp, easing: smooth});
  return <svg width="2800" height="1700" style={{position: 'absolute', left: 0, top: 0, pointerEvents: 'none'}}>
    <path d="M948 570 H1165 Q1210 570 1210 615 V660 Q1210 705 1255 705 H1375" fill="none" stroke="#3e3d3c" strokeWidth="2" />
    <path d="M948 570 H1165 Q1210 570 1210 615 V660 Q1210 705 1255 705 H1375" fill="none" stroke={C.ink} strokeWidth="3" pathLength="1" strokeDasharray="1" strokeDashoffset={1 - progress} />
    <path d="M1955 970 V1400 Q1955 1440 1915 1440 H1740 V1560" fill="none" stroke="#3e3d3c" strokeWidth="2" />
  </svg>;
};

const Repo: React.FC = () => {
  const f = useCurrentFrame();
  const reveal = interpolate(f, [315, 345], [0, 1], {...clamp, easing: smooth});
  return <div style={{position: 'absolute', left: 1120, top: 1570, width: 1260, color: C.ink}}>
    <div style={{display: 'flex', alignItems: 'center', gap: 28, marginBottom: 24}}>
      <CanvasImage src={staticFile('research/iphonecam.svg')} style={{width: 90, height: 90}} />
      <div style={{fontSize: 67, lineHeight: 1.12, fontWeight: 700}}>iPhone Camera<br /><span style={{fontSize: 42, color: C.dim}}>for Omarchy</span></div>
    </div>
    <div style={{marginTop: 55, display: 'flex', gap: 26, alignItems: 'center', opacity: reveal, translate: `0 ${18 * (1 - reveal)}px`}}>
      <CanvasImage src={staticFile('research/github.svg')} style={{width: 42, height: 42, background: '#fafcfb', borderRadius: 22, padding: 5}} />
      <div style={{fontSize: 41}}>github.com/Nikshay1/iphonecam</div>
    </div>
    <div style={{marginTop: 32, fontSize: 26, color: C.dim}}>Native Omarchy plugin + Arch package · MIT</div>
  </div>;
};

export const IPhoneCameraLaunch: React.FC = () => {
  const frame = useCurrentFrame();
  const {width, height} = useVideoConfig();
  const fx = interpolate(frame, T, X, {...clamp, easing: smooth});
  const fy = interpolate(frame, T, Y, {...clamp, easing: smooth});
  const zoom = interpolate(frame, T, Z, {...clamp, easing: smooth});
  return <AbsoluteFill style={{background: C.bg, color: C.ink, fontFamily: 'JetBrains', overflow: 'hidden'}}>
    <div style={{position: 'absolute', width: 2800, height: 1700, transform: `translate(${width / 2 - fx * zoom}px, ${height / 2 - fy * zoom}px) scale(${zoom})`, transformOrigin: '0 0'}}>
      <div style={{position: 'absolute', left: 180, top: 95, fontSize: 24, letterSpacing: 3, color: C.dim}}>IPHONE CAMERA / OMARCHY</div>
      <Connection />
      <SourcePanel />
      <Terminal />
      <Repo />
      <Pointer />
    </div>
    <div style={{position: 'absolute', left: 0, right: 0, bottom: 0, padding: '22px 52px 26px', background: C.bg, borderTop: '1px solid #343231', fontSize: 38, lineHeight: 1.35, color: '#c2bdba'}}>
      {frame < 344 ? <div style={{display: 'flex', justifyContent: 'space-between'}}><span>UI demonstration</span><span>DroidCam required</span></div> : <>
        <div>DroidCam on iPhone required.</div>
        <div style={{fontSize: 38}}>Remote lens controls may need Pro.</div>
      </>}
    </div>
    <Audio src={staticFile('sound.wav')} />
  </AbsoluteFill>;
};
