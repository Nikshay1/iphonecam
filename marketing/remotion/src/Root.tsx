import React from 'react';
import {Composition} from 'remotion';
import {IPhoneCameraLaunch} from './Composition';
export const RemotionRoot: React.FC = () => <Composition id="IPhoneCameraLaunch" component={IPhoneCameraLaunch} width={1440} height={1080} fps={30} durationInFrames={450} />;
