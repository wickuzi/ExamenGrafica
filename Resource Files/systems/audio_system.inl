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

void initAudio()
{
    audioCommand("close bgm");
    audioCommand("close footsteps");
    audioCommand("close interaction");

    if (audioCommand("open \"sounds\\walking_soft.wav\" type waveaudio alias footsteps"))
    {
        audioCommand("setaudio footsteps volume to 1000");
    }

    if (audioCommand("open \"sounds\\2-new-game.mp3\" type mpegvideo alias interaction"))
    {
        audioCommand("setaudio interaction volume to 1000");
    }
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

void updateFootstepAudio()
{
    if (playerIsMoving)
    {
        footstepTimer -= deltaTime;
        if (footstepTimer <= 0.0f)
        {
            audioCommand("stop footsteps");
            audioCommand("seek footsteps to start");
            if (audioCommand("play footsteps from 0"))
            {
                footstepsPlaying = true;
                footstepTimer = 0.60f;
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
    audioCommand("close footsteps");
    audioCommand("close interaction");
    audioCommand("close bgm");
    backgroundMusicPlaying = false;
}
