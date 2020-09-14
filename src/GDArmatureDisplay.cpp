#include "GDArmatureDisplay.h"

#include "GDDisplay.h"
#include "core/method_bind_ext.gen.inc"


GDArmatureDisplay::GDArmatureDisplay()
{
	p_armature = nullptr;
    set_use_parent_material(true);
}

GDArmatureDisplay::~GDArmatureDisplay()
{
	dispose(true);
}

void GDArmatureDisplay::_bind_methods() {
	CLASS_BIND_GODO::bind_method(METH("is_frozen"), &GDArmatureDisplay::is_frozen);
	CLASS_BIND_GODO::bind_method(METH("freeze"), &GDArmatureDisplay::freeze);
	CLASS_BIND_GODO::bind_method(METH("thaw"), &GDArmatureDisplay::thaw);
	CLASS_BIND_GODO::bind_method(METH("has_animation", "animation_name"), &GDArmatureDisplay::has_animation);
	CLASS_BIND_GODO::bind_method(METH("get_animations"), &GDArmatureDisplay::get_animations);
	CLASS_BIND_GODO::bind_method(METH("is_playing"), &GDArmatureDisplay::is_playing);
	CLASS_BIND_GODO::bind_method(METH("play", "animation_name", "loop_count"), &GDArmatureDisplay::play);
	CLASS_BIND_GODO::bind_method(METH("play_from_time", "animation_name", "f_time", "loop_count"), &GDArmatureDisplay::play_from_time);
	CLASS_BIND_GODO::bind_method(METH("play_from_progress", "animation_name", "f_progress", "loop_count"), &GDArmatureDisplay::play_from_progress);
	CLASS_BIND_GODO::bind_method(METH("stop", "animation_name", "b_reset"), &GDArmatureDisplay::stop);
	CLASS_BIND_GODO::bind_method(METH("stop_all_animations", "b_reset"), &GDArmatureDisplay::stop_all_animations);
	CLASS_BIND_GODO::bind_method(METH("fade_in"), &GDArmatureDisplay::fade_in);
	CLASS_BIND_GODO::bind_method(METH("has_slot", "slot_name"), &GDArmatureDisplay::has_slot);
	CLASS_BIND_GODO::bind_method(METH("get_slot", "slot_name"), &GDArmatureDisplay::get_slot);
	CLASS_BIND_GODO::bind_method(METH("get_slots"), &GDArmatureDisplay::get_slots);
	CLASS_BIND_GODO::bind_method(METH("reset"), &GDArmatureDisplay::reset);
	CLASS_BIND_GODO::bind_method(METH("set_flip_x", "is_flipped"), &GDArmatureDisplay::flip_x);
	CLASS_BIND_GODO::bind_method(METH("is_flipped_x"), &GDArmatureDisplay::is_flipped_x);
	CLASS_BIND_GODO::bind_method(METH("set_flip_y" , "is_flipped"), &GDArmatureDisplay::flip_y);
	CLASS_BIND_GODO::bind_method(METH("is_flipped_y"), &GDArmatureDisplay::is_flipped_y);
	CLASS_BIND_GODO::bind_method(METH("set_debug", "is_debug"), &GDArmatureDisplay::set_debug);
	CLASS_BIND_GODO::bind_method(METH("get_ik_constraints"), &GDArmatureDisplay::get_ik_constraints);
	CLASS_BIND_GODO::bind_method(METH("set_ik_constraint", "constraint_name", "new_position"), &GDArmatureDisplay::set_ik_constraint);
	CLASS_BIND_GODO::bind_method(METH("set_ik_constraint_bend_positive", "constraint_name", "is_positive"), &GDArmatureDisplay::set_ik_constraint_bend_positive);
	CLASS_BIND_GODO::bind_method(METH("get_bones"), &GDArmatureDisplay::get_bones);
	CLASS_BIND_GODO::bind_method(METH("get_bone", "bone_name"), &GDArmatureDisplay::get_bone);


	// Enum
	BIND_CONSTANT(ANIMATION_PROCESS_FIXED);
	BIND_CONSTANT(ANIMATION_PROCESS_IDLE);

	BIND_CONSTANT(FadeOut_None);
	BIND_CONSTANT(FadeOut_SameLayer);
	BIND_CONSTANT(FadeOut_SameGroup);
	BIND_CONSTANT(FadeOut_SameLayerAndGroup);
	BIND_CONSTANT(FadeOut_All);
	BIND_CONSTANT(FadeOut_Single);
}

bool GDArmatureDisplay::is_frozen() {
	return !is_physics_processing();
}

void GDArmatureDisplay::freeze() {
	set_physics_process(false);
}

void GDArmatureDisplay::thaw() {
	set_physics_process(true);
}


void GDArmatureDisplay::set_debug(bool _b_debug) {
	if (!p_armature)
		return;
	auto arr = p_armature->getSlots();
	for (auto item : arr) {
		if (!item) continue;

		auto display = item->getRawDisplay();
		if (!display) continue;
		static_cast<GDDisplay *>(display)->b_debug = _b_debug;
		static_cast<GDDisplay *>(display)->update();
	}

}
bool GDArmatureDisplay::has_animation(const String &_animation_name) {
	if (p_armature == nullptr || !getAnimation()) {
		return false;
	}

	return getArmature()->getArmatureData()->
		getAnimation(_animation_name.ascii().get_data()) != nullptr;
}

Array GDArmatureDisplay::get_animations() {
	Array animations{};

	const ArmatureData *data = p_armature->getArmatureData();

	for (std::string animation_name : data->getAnimationNames()) {
		animations.push_back(String(animation_name.c_str()));
	}

	return animations;
}

String GDArmatureDisplay::get_current_animation() const {
	if (!getAnimation())
		return String("");
	return String(getAnimation()->getLastAnimationName().c_str());
}

String GDArmatureDisplay::get_current_animation_on_layer(int _layer) const {
	if (!getAnimation())
		return String("");

	std::vector<AnimationState *> states = p_armature->getAnimation()->getStates();

	for (AnimationState *state : states) {
		if (state->layer == _layer) {
			return state->getName().c_str();
		}
	}

	return String("");
}

String GDArmatureDisplay::get_current_animation_in_group(const String &_group_name) const {
	if (!getAnimation())
		return String("");

	std::vector<AnimationState *> states = getAnimation()->getStates();

	for (AnimationState *state : states) {
		if (state->group == _group_name.ascii().get_data()) {
			return state->getName().c_str();
		}
	}

	return String("");
}

float GDArmatureDisplay::tell_animation(const String &_animation_name) {
	if (has_animation(_animation_name)) {
		AnimationState *animation_state = getAnimation()->getState(_animation_name.ascii().get_data());
		if (animation_state)
			return animation_state->getCurrentTime() / animation_state->_duration;
	}
	return 0;
}

void GDArmatureDisplay::seek_animation(const String &_animation_name, float progress) {
	if (has_animation(_animation_name)) {
		stop(_animation_name, true);
		auto current_progress = Math::fmod(progress, 1.0f);
		if (current_progress == 0 && progress != 0)
			current_progress = 1.0f;
		p_armature->getAnimation()->gotoAndStopByProgress(_animation_name.ascii().get_data(), current_progress < 0 ? 1. + current_progress : current_progress);
	}

}

bool GDArmatureDisplay::is_playing() const {
	return getAnimation()->isPlaying();
}

void GDArmatureDisplay::play(const String &_animation_name, int loop) {
	if (has_animation(_animation_name)) {
		getAnimation()->play(_animation_name.ascii().get_data(), loop);
	}
}

void GDArmatureDisplay::play_from_time(const String &_animation_name, float _f_time, int loop) {
	if (has_animation(_animation_name)) {
		play(_animation_name, loop);
		getAnimation()->gotoAndPlayByTime(_animation_name.ascii().get_data(), _f_time);
	}
}


void GDArmatureDisplay::play_from_progress(const String &_animation_name, float f_progress, int loop) {
	if (has_animation(_animation_name)) {
		play(_animation_name, loop);
		getAnimation()->gotoAndPlayByProgress(_animation_name.ascii().get_data(), f_progress);
	}
}

void GDArmatureDisplay::stop(const String &_animation_name, bool b_reset) {
	if (getAnimation()) {
		getAnimation()->stop(_animation_name.ascii().get_data());

		if (b_reset) {
			reset();
		}
	}	
}

void GDArmatureDisplay::stop_all_animations(bool b_children, bool b_reset) {
	if (getAnimation()) {
		getAnimation()->stop("");
	}

	if (b_reset) {
		reset();
	}

	if (b_children) {
		for (Slot *slot : getArmature()->getSlots()) {
			if (slot->getDisplayList().size() == 0) continue;
			std::pair<void *, DisplayType> display = slot->getDisplayList()[slot->getDisplayIndex()];
			if (display.second == DisplayType::Armature) {
				Armature *armature = static_cast<Armature *>(display.first);
				GDArmatureDisplay *convertedDisplay = static_cast<GDArmatureDisplay *>(armature->getDisplay());
				convertedDisplay->stop_all_animations(b_children, b_reset);
			}
		}
	}
}

void GDArmatureDisplay::fade_in(const String &_animation_name, float _time, int _loop, int _layer, const String &_group, GDArmatureDisplay::AnimFadeOutMode _fade_out_mode) {
	if (has_animation(_animation_name)) {
		getAnimation()->fadeIn(_animation_name.ascii().get_data(), _time, _loop, _layer, _group.ascii().get_data(), (AnimationFadeOutMode)_fade_out_mode);
	}
}

void GDArmatureDisplay::reset() {
	if (getAnimation()) {
		getAnimation()->reset();
	}
}

bool GDArmatureDisplay::has_slot(const String &_slot_name) const {
	return getArmature()->getSlot(_slot_name.ascii().get_data()) != nullptr;
}

Dictionary GDArmatureDisplay::get_slots() {
	Dictionary slots{};

	for (auto &slot : _slots) {
		slots[slot.first.c_str()] = slot.second;
	}

	return slots;
}

GDSlot *GDArmatureDisplay::get_slot(const String &_slot_name) {
	
	return _slots[_slot_name.ascii().get_data()];
}

void GDArmatureDisplay::flip_x(bool _b_flip) {
	getArmature()->setFlipX(_b_flip);
	getArmature()->advanceTime(0);
}

bool GDArmatureDisplay::is_flipped_x() const {
	return getArmature()->getFlipX();
}

void GDArmatureDisplay::flip_y(bool _b_flip) {
	getArmature()->setFlipY(_b_flip);
	getArmature()->advanceTime(0);
}

bool GDArmatureDisplay::is_flipped_y() const {
	return getArmature()->getFlipY();
}

Dictionary GDArmatureDisplay::get_ik_constraints() {
	Dictionary dict;

	for (auto &constraint : getArmature()->getArmatureData()->constraints) {
		dict[String(constraint.first.c_str())] = Vector2(constraint.second->target->transform.x, constraint.second->target->transform.y);
	}

	return dict;
}

void GDArmatureDisplay::set_ik_constraint(const String &name, Vector2 position){
	for (dragonBones::Constraint *constraint : getArmature()->_constraints) {
		if (constraint->getName() == name.ascii().get_data()) {
			dragonBones::BoneData *target = const_cast<BoneData *>(constraint->_constraintData->target);
			target->transform.x = position.x;
			target->transform.y = position.y;

			constraint->_constraintData->setTarget(target);
			constraint->update();
			getArmature()->invalidUpdate(target->name, true);
		}
	}
}

void GDArmatureDisplay::set_ik_constraint_bend_positive(const String &name, bool bend_positive) {
	for (dragonBones::Constraint *constraint : getArmature()->_constraints) {
		if (constraint->getName() == name.ascii().get_data()) {
			dragonBones::BoneData *target = const_cast<BoneData *>(constraint->_constraintData->target);

			static_cast<IKConstraint *>(constraint)->_bendPositive = bend_positive;
			constraint->update();
			getArmature()->invalidUpdate(target->name, true);
		}
	}
}

Dictionary GDArmatureDisplay::get_bones() {
	Dictionary bones{};

	for (auto &bone : _bones) {
		bones[bone.first.c_str()] = bone.second;
	}

	return bones;
}

GDBone2D *GDArmatureDisplay::get_bone(const String &name) {
	return _bones[name.ascii().get_data()];
}

Slot *GDArmatureDisplay::getSlot(const std::string &name) const {
	return p_armature->getSlot(name);
}

void GDArmatureDisplay::add_bone(std::string name, GDBone2D *new_bone) {
	_bones.insert(std::make_pair(name, new_bone));
	add_child(new_bone);
}

void GDArmatureDisplay::add_slot(std::string name, GDSlot *new_slot) {
	_slots.insert(std::make_pair(name, new_slot));
	add_child(new_slot);
}

void GDArmatureDisplay::dbInit(Armature* _p_armature)
{
    p_armature = _p_armature;
}

void GDArmatureDisplay::dbClear()
{
	p_armature = nullptr;
}

void GDArmatureDisplay::dbUpdate()
{
}

void GDArmatureDisplay::dispatchDBEvent(const std::string& _type, EventObject* _value)
{
    if(p_owner)
        p_owner->dispatch_event(String(_type.c_str()), _value);
}

void GDArmatureDisplay::dispose(bool _disposeProxy)
{
	if (p_armature)
	{
		p_armature->dispose();

		p_armature = nullptr;
	}
}

void GDArmatureDisplay::add_parent_class(bool _b_debug, const Ref<Texture>& _m_texture_atlas)
{
    if(!p_armature)
        return;
    auto arr = p_armature->getSlots();



    for (auto item : arr)
    {
        if (!item) continue;

        auto display = item->getRawDisplay();
        if (!display) continue;

		for (std::pair<void *, DisplayType> displayItem : item->getDisplayList()) {
			// propagate texture to child armature slots?
			if (displayItem.second == DisplayType::Armature) {
				// recurse your way on down there, you scamp
				Armature *armature = static_cast<Armature *>(displayItem.first);
				GDArmatureDisplay *armatureDisplay = static_cast<GDArmatureDisplay*>(armature->getDisplay());
				armatureDisplay->p_owner = p_owner;
				armatureDisplay->add_parent_class(b_debug, _m_texture_atlas);
				continue;
			}
		}

        add_child(static_cast<GDDisplay*>(display));
        static_cast<GDDisplay*>(display)->p_owner = p_owner;
        static_cast<GDDisplay*>(display)->b_debug = _b_debug;
        static_cast<GDDisplay*>(display)->texture = _m_texture_atlas;


    }
}

void GDArmatureDisplay::update_childs(bool _b_color, bool _b_blending)
{
    if(!p_armature)
        return;

    auto arr = p_armature->getSlots();
    for (auto item : arr)
    {
        if (!item) continue;

        if(_b_color)
            item->_colorDirty = true;

        if(_b_blending)
            item->invalidUpdate();

        item->update(0);
    }
}

void GDArmatureDisplay::update_material_inheritance(bool _b_inherit_material)
{
    if(!p_armature)
        return;

    auto arr = p_armature->getSlots();
    for (auto item : arr)
    {
        if (!item) continue;
        auto display = item->getRawDisplay();
        if (!display) continue;

        static_cast<GDDisplay*>(display)->set_use_parent_material(_b_inherit_material);
    }
}


void GDArmatureDisplay::update_texture_atlas(const Ref<Texture>& _m_texture_atlas)
{
    if(!p_armature)
        return;

    auto arr = p_armature->getSlots();
    for (auto item : arr)
    {
        if (!item) continue;
        auto display = item->getRawDisplay();
        if (!display) continue;

        static_cast<GDDisplay*>(display)->texture = _m_texture_atlas;
        static_cast<GDDisplay*>(display)->update();
    }
}


