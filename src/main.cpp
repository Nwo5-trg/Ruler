#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <random>

using namespace geode::prelude;

auto mod = Mod::get();

std::string floatToFormatedNumber(float input, int precision) {
    std::stringstream stringStream;
    stringStream << std::fixed << std::setprecision(precision) << input;
    std::string string = stringStream.str();
    string.erase(string.find_last_not_of('0') + 1, std::string::npos);
    if (string.back() == '.') string.pop_back();
    if (!(string.find_first_of("0123456789") != std::string::npos)) string = "1";
    return string;
}

class $modify(Editor, LevelEditorLayer) {
    struct Fields {
        CCLayer* measureRenderLayer;
        CCLayer* batchLayer;
        std::vector<cocos2d::ccColor3B> colors {
            ccc3(0, 255, 162), ccc3(56, 89, 255), ccc3(166, 255, 0),
            ccc3(255, 0, 162), ccc3(247, 81, 81), ccc3(109, 47, 255),
            ccc3(0, 208, 255), ccc3(255, 0, 0), ccc3(255, 221, 0),
            ccc3(255, 73, 234), ccc3(0, 201, 20), ccc3(17, 255, 0),
            ccc3(255, 64, 0), ccc3(145, 51, 199), ccc3(145, 194, 255),
            ccc3(255, 196, 0), { 255, 145, 0 }, ccc3(0, 0, 255)

        };
    };

	bool init(GJGameLevel* p0, bool p1) {	
		if (!LevelEditorLayer::init(p0, p1)) return false;
            if (auto shaderLayer = this->getChildByType<ShaderLayer>(0)) m_fields->batchLayer = shaderLayer->getChildByType<CCNode>(1)->getChildByType<CCLayer>(0);
            else m_fields->batchLayer = this->getChildByType<CCNode>(1)->getChildByType<CCLayer>(0);

            auto measureRenderLayer = CCLayer::create();
            measureRenderLayer->setPosition(ccp(0, 0));
            measureRenderLayer->setZOrder(9999);
            measureRenderLayer->setID("measure-render-layer");
            m_fields->batchLayer->addChild(measureRenderLayer);
            m_fields->measureRenderLayer = measureRenderLayer;
        return true;
    }

    void createMeasurement(CCArray* objs, float gridSize) {
        if (objs->count() >= 2) {
            auto pos1 = getLowestPoint(objs);
            auto pos2 = getHighestPoint(objs);
            auto color = m_fields->colors[rand() % m_fields->colors.size()];
            auto measurementContainer = CCNode::create();
            m_fields->measureRenderLayer->addChild(measurementContainer);
            measurementContainer->setPosition(ccp(0, 0));
            measurementContainer->setID("measurement-" + std::to_string(m_fields->measureRenderLayer->getChildren()->count()));

            if (mod->getSettingValue<bool>("fill-enabled")) createFill(pos1, pos2, gridSize, measurementContainer, color);
            if (mod->getSettingValue<bool>("outline-enabled")) createEdges(pos1, pos2, gridSize, measurementContainer, color);
            if (mod->getSettingValue<bool>("corners-enabled")) createCorners(pos1, pos2, gridSize, measurementContainer, color);
            if (mod->getSettingValue<bool>("label-enabled")) createLabels(objs, pos1, pos2, gridSize, measurementContainer, color);

            if (mod->getSettingValue<bool>("delete-enabled")) m_editorUI->onDeleteSelected(nullptr);
            else if (mod->getSettingValue<bool>("deselect-enabled")) m_editorUI->deselectAll();

        }
        else if (objs->count() == 0) {
            m_fields->measureRenderLayer->removeAllChildren();
        }
    }

    void createFill(CCPoint obj1Pos, CCPoint obj2Pos, float gridSize, CCNode* container, ccColor3B color) {
        float scaleX = (fabs(obj2Pos.x - obj1Pos.x) + gridSize) / 30;
        float scaleY = (fabs(obj2Pos.y - obj1Pos.y) + gridSize) / 30;
        
        auto fill = CCSprite::create("fill.png"_spr);
        fill->setOpacity(mod->getSettingValue<int64_t>("fill-opacity"));
        fill->setPosition(ccp((obj1Pos.x + obj2Pos.x) / 2, (obj1Pos.y + obj2Pos.y) / 2));
        fill->setScaleX(scaleX);
        fill->setScaleY(scaleY);
        fill->setColor(mod->getSettingValue<bool>("random-color") &&  !mod->getSettingValue<bool>("fill-override-random-color") ? color : mod->getSettingValue<cocos2d::ccColor3B>("fill-color"));
        container->addChild(fill);
    }

    void createEdges(CCPoint obj1Pos, CCPoint obj2Pos, float gridSize, CCNode* container, ccColor3B color) {
        float scaleX = (fabs(obj2Pos.x - obj1Pos.x) + gridSize) / 30;
        float scaleY = (fabs(obj2Pos.y - obj1Pos.y) + gridSize) / 30;
        auto col1 = mod->getSettingValue<bool>("random-color") && !mod->getSettingValue<bool>("outline-override-random-color") ? color : mod->getSettingValue<cocos2d::ccColor3B>("outline-color-one");
        auto col2 = mod->getSettingValue<cocos2d::ccColor3B>("outline-color-two");
        auto gradientDirection = mod->getSettingValue<int64_t>("gradient-direction") * 90;

        auto createEdges = [&] (bool gradient) {
            if (mod->getSettingValue<bool>("random-color")) gradient = false;
            auto createEdge = [&] (CCPoint pos, int rotation, bool axis) {
                auto edge = CCSprite::create("edge.png"_spr);
                edge->setOpacity(mod->getSettingValue<int64_t>("outline-opacity"));
                edge->setRotation(rotation);
                edge->setPosition(pos);
                edge->setScaleX(axis ? scaleY : scaleX);
                edge->setScaleY(gridSize / 30);
                if (gradient) edge->setColor(rotation == gradientDirection ? col1 : col2);
                else edge->setColor(col1);
                container->addChild(edge);

                if (gradient) { // ive given up on making this fucking gradient thing "good" i just want it to work
                    auto createGradientEdge = [&] (bool flip, bool gradientAxis) {
                        auto gradientEdge = CCSprite::create("edgegradient.png"_spr);
                        gradientEdge->setOpacity(mod->getSettingValue<int64_t>("outline-opacity"));
                        gradientEdge->setRotation(rotation);
                        gradientEdge->setPosition(pos);
                        gradientEdge->setScaleX(gradientAxis ? (flip ? (axis ? scaleY : scaleX) : (axis ? -scaleY : -scaleX)) : (flip ? (axis ? -scaleY : -scaleX) : (axis ? scaleY : scaleX)));
                        gradientEdge->setScaleY(gridSize / 30);
                        gradientEdge->setColor(col1);
                        container->addChild(gradientEdge);
                    };
                    if (gradientDirection == 180 || gradientDirection == 360) {
                        if (gradientDirection == 360 ? rotation == 270 : rotation == 90) createGradientEdge(false, true);
                        else if (rotation == 270 || rotation == 90) createGradientEdge(true, true);
                    }
                    else if (gradientDirection == 90 || gradientDirection == 270) {
                        if (gradientDirection == 270 ? rotation == 360 : rotation == 180) createGradientEdge(false, false);
                        else if (rotation == 360 || rotation == 180) createGradientEdge(true, false);
                    }
                }
            };
            createEdge(ccp(std::max(obj1Pos.x, obj2Pos.x), (obj1Pos.y + obj2Pos.y) / 2), 90, true);
            createEdge(ccp(std::min(obj1Pos.x, obj2Pos.x), (obj1Pos.y + obj2Pos.y) / 2), 270, true);
            createEdge(ccp((obj1Pos.x + obj2Pos.x) / 2, std::max(obj1Pos.y, obj2Pos.y)), 360, false);
            createEdge(ccp((obj1Pos.x + obj2Pos.x) / 2, std::min(obj1Pos.y, obj2Pos.y)), 180, false);
        };
        
        if (mod->getSettingValue<bool>("gradient-enabled")) createEdges(true);
        else createEdges(false);
    }

    void createCorners(CCPoint obj1Pos, CCPoint obj2Pos, float gridSize, CCNode* container, ccColor3B color) {
        std::vector<CCPoint> points {
            obj1Pos, obj2Pos
        };
        for (auto point : points) {
            auto corner = CCSprite::create("corner.png"_spr);
            corner->setOpacity(mod->getSettingValue<int64_t>("corner-opacity"));
            corner->setPosition(point);
            corner->setScale(gridSize / 30);    
            corner->setColor(mod->getSettingValue<bool>("random-color") && !mod->getSettingValue<bool>("corner-override-random-color") ? color : mod->getSettingValue<cocos2d::ccColor3B>("corner-color"));
            container->addChild(corner);
        }
    }

    void createLabels(CCArray* objs, CCPoint obj1Pos, CCPoint obj2Pos, float gridSize, CCNode* container, ccColor3B color) {
        auto measureType = mod->getSettingValue<std::string>("measure-type");
        auto measureDisplayType = mod->getSettingValue<std::string>("measure-display-type");
        auto rounding = mod->getSettingValue<int64_t>("label-rounding");
        int subAmount = 0;
        if (measureType == "Full") subAmount = -1;
        if (measureType == "Inbetween") subAmount = 1;
        float buffer = mod->getSettingValue<double>("label-buffer");
        float measureX = (fabs(obj2Pos.x - obj1Pos.x) - (gridSize * subAmount)) / gridSize;
        float measureY = (fabs(obj2Pos.y - obj1Pos.y) - (gridSize * subAmount)) / gridSize;
        float fontScale;
        int fontNumber = mod->getSettingValue<int64_t>("label-font");
        std::string font;

        if (fontNumber == 0) {
            font = "bigFont.fnt";
            fontScale = 0.75;
        } else if (fontNumber <= 59) {
            font = std::string("gjFont") + (fontNumber < 10 ? "0" : "") + std::to_string(fontNumber) + ".fnt";
            fontScale = 0.75;
        } else if (fontNumber == 60) {
            font = "chatFont.fnt";
            fontScale = 1.75;
        } else if (fontNumber == 61) {
            font = "goldFont.fnt";
            fontScale = 1;
        }

        std::vector<std::pair<float, CCPoint>> measures {
            {measureX, ccp((obj1Pos.x + obj2Pos.x) / 2, std::max(obj1Pos.y, obj2Pos.y) + gridSize + buffer)},
            {measureY, ccp(std::max(obj1Pos.x, obj2Pos.x) + gridSize + buffer, (obj1Pos.y + obj2Pos.y) / 2)}
        };

        for (auto& [measure, pos] : measures) {
            std::string string;
            if (measureDisplayType == "Decimal") string = floatToFormatedNumber(measure, rounding);

            if (measureDisplayType == "GD Units") {
                string = (measure * gridSize) - (std::floor(measure) * gridSize) == 0 ? floatToFormatedNumber(std::floor(measure), rounding) : floatToFormatedNumber(std::floor(measure), rounding) + mod->getSettingValue<std::string>("gd-unit-seperator") + floatToFormatedNumber((measure * gridSize) - (std::floor(measure) * gridSize), rounding);
            }
            auto label = CCLabelBMFont::create(string.c_str(), font.c_str());
            label->setOpacity(mod->getSettingValue<int64_t>("label-opacity"));
            label->setPosition(measure == measureY ? ccp(pos.x + (label->getContentSize().width / 2), pos.y) : pos);
            label->setScale(mod->getSettingValue<double>("label-scale") * fontScale);
            label->setColor(mod->getSettingValue<bool>("random-color") && !mod->getSettingValue<bool>("label-override-random-color") ? color : mod->getSettingValue<cocos2d::ccColor3B>("fill-color"));
            container->addChild(label);
        }
    }

    CCPoint getLowestPoint (CCArray* objs) {
        CCPoint lowestPoint = ccp(FLT_MAX, FLT_MAX);
        for (int i = 0; i < objs->count(); i++) {
            auto obj = static_cast<GameObject*>(objs->objectAtIndex(i));
            auto pos = obj->getPosition();
            if (pos.x < lowestPoint.x) {
                lowestPoint = ccp(pos.x, lowestPoint.y);
            }
            if (pos.y < lowestPoint.y) {
                lowestPoint = ccp(lowestPoint.x, pos.y);
            }
        }
        return lowestPoint;
    }

    CCPoint getHighestPoint (CCArray* objs) {
        CCPoint highestPoint = ccp(-FLT_MAX, -FLT_MAX);
        for (int i = 0; i < objs->count(); i++) {
            auto obj = static_cast<GameObject*>(objs->objectAtIndex(i));
            auto pos = obj->getPosition();
            if (pos.x > highestPoint.x) {
                highestPoint = ccp(pos.x, highestPoint.y);
            }
            if (pos.y > highestPoint.y) {
                highestPoint = ccp(highestPoint.x, pos.y);
            }
        }
        return highestPoint;
    }
};

class $modify(EditUI, EditorUI) {
    struct Fields {
        bool measureButtonDoubleClick;
    };
	void createMoveMenu() {
		EditorUI::createMoveMenu();
        auto* btn = this->getSpriteButton("Sheet.png"_spr, menu_selector(EditUI::onCreateMeasurement), nullptr, 1);
        m_editButtonBar->m_buttonArray->addObject(btn);
        auto rows = GameManager::sharedState()->getIntGameVariable("0049");
        auto cols = GameManager::sharedState()->getIntGameVariable("0050");
        m_editButtonBar->reloadItems(rows, cols);
	}
    
    void onCreateMeasurement(CCObject* sender) {
        static_cast<Editor*>(LevelEditorLayer::get())->createMeasurement(this->getSelectedObjects(), getGridSizeFromSelectedObjects());
    }

    float getGridSizeFromSelectedObjects() {
        this->updateGridNodeSize();
        return m_gridSize;
    }
};