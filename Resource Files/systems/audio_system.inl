bool audioCommand(const std::string &command)
{
    char errorText[256] = {};
    MCIERROR error = mciSendStringA(command.c_str(), nullptr, 0, nullptr);
    if (error != 0)
    {
        mciGetErrorStringA(error, errorText, sizeof(errorText));
        std::cout << "Audio command failed: " << command << " -> " << errorText << std::endl;
        return false;
    }
    return true;
}

bool pyramidInterferencePlaying = false;

void initAudio()
{
    audioCommand("close bgm");
    audioCommand("close footsteps");
    audioCommand("close interaction");
    audioCommand("close jameshurt");
    audioCommand("close angelacry");
    audioCommand("close interference");
    audioCommand("close shotgunfire");
    audioCommand("close pyramidhit");

    if (audioCommand("open \"sounds\\walking_soft.wav\" type waveaudio alias footsteps"))
    {
        audioCommand("setaudio footsteps volume to 1000");
    }

    if (audioCommand("open \"sounds\\2-new-game.mp3\" type mpegvideo alias interaction"))
    {
        audioCommand("setaudio interaction volume to 1000");
    }

    if (audioCommand("open \"models\\james\\jamesounds\\jameshurt.wav\" type waveaudio alias jameshurt"))
        audioCommand("setaudio jameshurt volume to 1000");

    if (audioCommand("open \"models\\angela\\sounds\\crying-sfx.mp3\" type mpegvideo alias angelacry"))
        audioCommand("setaudio angelacry volume to 1000");

    if (audioCommand("open \"sounds\\6-interference.mp3\" type mpegvideo alias interference"))
        audioCommand("setaudio interference volume to 720");

    if (audioCommand("open \"models\\james\\gunanimations\\shotgun-4.mp3\" type mpegvideo alias shotgunfire"))
        audioCommand("setaudio shotgunfire volume to 1000");

    if (audioCommand("open \"models\\enemies\\pyramidhead\\53-monster-dead.mp3\" type mpegvideo alias pyramidhit"))
        audioCommand("setaudio pyramidhit volume to 1000");
}

void startBackgroundMusic()
{
    if (backgroundMusicPlaying)
        return;

    audioCommand("close bgm");
    if (audioCommand("open \"sounds\\13-wiltse-road-road-to-silent-hill.mp3\" type mpegvideo alias bgm"))
    {
        audioCommand("setaudio bgm volume to 400");
        if (audioCommand("play bgm repeat"))
            backgroundMusicPlaying = true;
    }
}

void playInteractionSound()
{
    audioCommand("stop interaction");
    audioCommand("seek interaction to start");
    audioCommand("play interaction from 0");
}

void playJamesHurtSound()
{
    audioCommand("stop jameshurt");
    audioCommand("seek jameshurt to start");
    audioCommand("play jameshurt from 0");
}

void playAngelaCrySound()
{
    audioCommand("stop angelacry");
    audioCommand("seek angelacry to start");
    audioCommand("play angelacry from 0");
}

void playShotgunSound()
{
    audioCommand("stop shotgunfire");
    audioCommand("seek shotgunfire to start");
    audioCommand("play shotgunfire from 0");
}

void playPyramidHeadHitSound()
{
    audioCommand("stop pyramidhit");
    audioCommand("seek pyramidhit to start");
    audioCommand("play pyramidhit from 0");
}

void setPyramidInterferenceActive(bool active)
{
    if (active == pyramidInterferencePlaying)
        return;

    pyramidInterferencePlaying = active;
    if (active)
    {
        audioCommand("stop bgm");
        audioCommand("close bgm");
        backgroundMusicPlaying = false;
        audioCommand("seek interference to start");
        audioCommand("play interference repeat");
    }
    else
    {
        audioCommand("stop interference");
        audioCommand("seek interference to start");
        startBackgroundMusic();
    }
}

void stopGameplayMusic()
{
    audioCommand("stop bgm");
    audioCommand("close bgm");
    audioCommand("stop interference");
    audioCommand("seek interference to start");
    backgroundMusicPlaying = false;
    pyramidInterferencePlaying = false;
}

void stopGameplayAudio()
{
    audioCommand("stop footsteps");
    audioCommand("seek footsteps to start");
    audioCommand("stop interaction");
    audioCommand("seek interaction to start");
    audioCommand("stop jameshurt");
    audioCommand("seek jameshurt to start");
    audioCommand("stop angelacry");
    audioCommand("seek angelacry to start");
    audioCommand("stop shotgunfire");
    audioCommand("seek shotgunfire to start");
    audioCommand("stop pyramidhit");
    audioCommand("seek pyramidhit to start");
    stopGameplayMusic();
    footstepsPlaying = false;
    footstepTimer = 0.0f;
}

void updateFootstepAudio(bool running)
{
    if (playerIsMoving)
    {
        const float stepInterval = running ? 0.34f : 0.60f;
        footstepTimer -= deltaTime;
        if (footstepTimer <= 0.0f)
        {
            audioCommand("stop footsteps");
            audioCommand("seek footsteps to start");
            if (audioCommand("play footsteps from 0"))
            {
                footstepsPlaying = true;
                footstepTimer = stepInterval;
            }
        }
    }
    else if (footstepsPlaying)
    {
        audioCommand("stop footsteps");
        audioCommand("seek footsteps to start");
        footstepsPlaying = false;
        footstepTimer = 0.0f;
    }
}

void shutdownAudio()
{
    audioCommand("stop footsteps");
    audioCommand("stop interaction");
    audioCommand("stop bgm");
    audioCommand("stop jameshurt");
    audioCommand("stop angelacry");
    audioCommand("stop interference");
    audioCommand("stop shotgunfire");
    audioCommand("stop pyramidhit");
    audioCommand("close footsteps");
    audioCommand("close interaction");
    audioCommand("close bgm");
    audioCommand("close jameshurt");
    audioCommand("close angelacry");
    audioCommand("close interference");
    audioCommand("close shotgunfire");
    audioCommand("close pyramidhit");
    backgroundMusicPlaying = false;
    pyramidInterferencePlaying = false;
}
