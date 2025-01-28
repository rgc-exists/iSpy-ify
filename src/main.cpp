#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/loader/GameEvent.hpp> 

#include "colors.cpp"

#include <math.h>

static bool flashingEnabled = false;
static bool blinkingEyeEnabled = true;

static CCSprite* blinkingEye;
static float blinkingTimer = 9999;
static bool blinking = false;
static bool eyeAppearing = false;
static bool eyeClosed = false;
static const char* blinkAnimation[8] = { "1.png"_spr, "2.png"_spr, "3.png"_spr, "4.png"_spr, "5.png"_spr, "5.png"_spr, "5.png"_spr, "5.png"_spr };
static CCSprite* overlaySprite;


static ccColor3B randColorRGB(ccColor3B curColor, int addBrightness = 0) {
	ccColor3B multColor = ccc3(rand() % (255 - addBrightness) + addBrightness, rand() % (255 - addBrightness) + addBrightness, rand() % (255 - addBrightness) + addBrightness);
	ccColor3B finalColor = ccc3((curColor.r + multColor.r) / 2, (curColor.g + multColor.g) / 2, (curColor.b + multColor.b) / 2);
	return finalColor;
}
static ccColor3B randColor(float sat = 255, float bright = 255, float absSat = 255, float absBright = 255) {
	ccHSVValue hsv = cchsv(rand() % 360, sat, bright, absSat, absBright);
	ccColor3B finalColor = hsv2rgb(hsv);
	return finalColor;
}
static ccHSVValue randColorHSV(float sat = 255, float bright = 255, float absSat = 255, float absBright = 255) {
	ccHSVValue hsv = cchsv(rand() % 360, sat, bright, absSat, absBright);
	return hsv;
}

#ifdef GEODE_IS_WINDOWS
// This is only CONFIRMED to cause no problems on Windows. On Android it causes issues, idk about Mac.
class $modify(CCSprite) {
	virtual void draw() {
		if (flashingEnabled && this != blinkingEye) {
			if (overlaySprite) {
				if (this == overlaySprite) {
					CCSprite::draw();
					return;
				}
			}

			ccColor3B newColor = randColor();
			setColor(newColor);
			updateDisplayedColor(newColor);

			CCNodeRGBA* node = (CCNodeRGBA*)this;
			node->setColor(newColor);
			node->updateDisplayedColor(newColor);
		}
		CCSprite::draw();

	}
};
#endif

class $modify(PlayLayer) {
	struct Fields {
		bool m_levelIsPlatformer;
	};

	void resetBlinking() {
		blinking = false;
		eyeClosed = false;
		blinkingEye->removeFromParentAndCleanup(true);
		blinkingTimer = rand() % 30 * 14 + 14;
	}

	virtual void postUpdate(float p0) {
		PlayLayer::postUpdate(p0);

		PlayerObject* player = GJBaseGameLayer::get()->m_player1;
		if (!player) return;
		if (flashingEnabled && !m_isPaused && !player->m_isDead) {
			if (overlaySprite) {
				if (eyeClosed) overlaySprite->setVisible(true);
				else overlaySprite->setVisible(false);
			}


			CCSize winSize = CCDirector::sharedDirector()->getWinSize();
			if (!blinking) {
				if (!overlaySprite) return;
				if (blinkingTimer <= 0 && blinkingEyeEnabled && !m_fields->m_levelIsPlatformer) {
					blinking = true;
					eyeAppearing = true;
					blinkingEye = CCSprite::create("1.png"_spr);
					UILayer::get()->addChild(blinkingEye);
					blinkingEye->setScale(2);
					auto frame = CCTextureCache::sharedTextureCache()->addImage("5.png"_spr, true);
					blinkingEye->setTexture(frame);

					overlaySprite->setOpacity((rand() % 50) + 175);
				}
			}
			else if (eyeAppearing) {
				float heightOffset = blinkingTimer + 21;
				blinkingEye->setPosition(ccp(winSize.width * .75f, winSize.height * .75f + heightOffset * 6.5f));
				log::debug("{}", heightOffset);
				if (heightOffset < 0) {
					eyeAppearing = false;
					FMODAudioEngine::get()->playEffect("iSpy_Blinking.wav"_spr);
				}
			} else {
				int animIndex = abs(fmodf(ceilf(blinkingTimer * 8) / 8 - 64, 7));
				auto frame = CCTextureCache::sharedTextureCache()->addImage(blinkAnimation[animIndex], true);
				blinkingEye->setTexture(frame);
				blinkingEye->setPosition(winSize * .75f);
				eyeClosed = (animIndex > 3);

				if (blinkingTimer <= -64 - 21) {
					blinkingEye->removeFromParentAndCleanup(true);
					resetBlinking();
				}
			}

			blinkingTimer -= CCDirector::sharedDirector()->getActualDeltaTime() * 7.0348 * 2;
		}
	}


	void startGame() {
		PlayLayer::startGame();

		if (blinking) return;

		blinkingEyeEnabled = !(Mod::get()->getSettingValue<bool>("blinking-eye-disabled"));

		blinkingTimer = rand() % 30 * 14 + 14;

		overlaySprite = CCSprite::create("square.png");
		UILayer::get()->addChild(overlaySprite);
		CCSize winSize = CCDirector::sharedDirector()->getWinSize();
		overlaySprite->setPosition(winSize / 2);
		overlaySprite->setScaleX(winSize.width);
		overlaySprite->setScaleY(winSize.height);
		overlaySprite->setColor(ccc3(0, 0, 0));
		overlaySprite->setVisible(false);

	}

	void onQuit() {
		PlayLayer::onQuit();
		eyeClosed = false;
		blinking = false;
		blinkingTimer = INT_MAX;
		overlaySprite = nullptr;
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		if (blinking) {
			resetBlinking();
		}
	}

	void resetLevelFromStart() {
		PlayLayer::resetLevelFromStart();

		if (blinking) {
			resetBlinking();
		}
	}

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
		m_fields->m_levelIsPlatformer = level->isPlatformer();
		return true;
	}
};


class $modify(GameObjHook, GameObject) {
	virtual void updateMainColor(cocos2d::ccColor3B const& color) {
		if (flashingEnabled) {
			ccHSVValue curColorHSV = rgb2hsv(color);
			ccColor3B newColor = randColor(rand() % 50 + curColorHSV.s, rand() % 50 + curColorHSV.v, rand() % 50 + curColorHSV.absoluteSaturation, rand() % 50 + curColorHSV.absoluteBrightness);
			GameObject::updateMainColor(newColor);
		}
		else {
			GameObject::updateMainColor(color);
		}
	}
	virtual void updateSecondaryColor(cocos2d::ccColor3B const& color) {
		if (flashingEnabled) {
			ccHSVValue curColorHSV = rgb2hsv(color);
			ccColor3B newColor = randColor(rand() % 50 + curColorHSV.s, rand() % 50 + curColorHSV.v, rand() % 50 + curColorHSV.absoluteSaturation, rand() % 50 + curColorHSV.absoluteBrightness);
			GameObject::updateSecondaryColor(newColor);
		}
		else {
			GameObject::updateSecondaryColor(color);
		}
	}
};


$execute{

	listenForSettingChanges("blinking-eye-disabled", [](bool value) {
		blinkingEyeEnabled = !value;
	});
	listenForSettingChanges("mod-enabled", [](bool value) {
		flashingEnabled = value;
	});


	new EventListener<EventFilter<GameEvent>>(+[](GameEvent* ev) {
		if (!Mod::get()->getSettingValue<bool>("mod-enabled")) {
			flashingEnabled = false;
			return ListenerResult::Stop;
		}
		if (ev->getType() == GameEventType::Loaded) {
			geode::createQuickPopup(
				"EPILEPSY WARNING!!!",
				"The \"iSpy-ify\" mod causes EXTREME flashing lights and colors as its main joke.\n\nIF YOU ARE SENSETIVE TO FLASHING COLORS, PLEASE DISABLE THE MOD NOW.",
				"DON'T ENABLE FLASHING!", "Enable flashing.",
				[](auto, bool btn2) {
					if (btn2) {
						flashingEnabled = true;
					}
				}
			);
		}
		return ListenerResult::Stop;
	});
}