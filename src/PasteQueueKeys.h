#pragma once
// The paste queue's rule for one key event seen by its keyboard hook (MainFrm.cpp): hold Ctrl and tap V, V, V.
// Keys Bulb sends itself are "injected" and ignored, so its own paste never runs the queue and the Ctrl release
// that paste sends does not end the user's held Ctrl. Checked by tests\PasteQueueKeysTest.cpp.
struct PasteQueueKeys
{
	bool leftCtrl = false;
	bool rightCtrl = false;
	bool vDown = false;

	enum Result { Pass, Swallow, PasteNext };

	// otherModifiers: Shift, Alt or Win held; exactly Ctrl+V runs the queue, as Windows' own paste
	Result OnKey(unsigned vk, bool down, bool injected, bool otherModifiers)
	{
		if (injected)
		{
			return Pass;
		}
		if (vk == 0xA2) leftCtrl = down;  // VK_LCONTROL
		if (vk == 0xA3) rightCtrl = down; // VK_RCONTROL
		if (vk != 'V')
		{
			return Pass;
		}
		bool wasDown = vDown;
		vDown = down;
		if ((leftCtrl || rightCtrl) == false || otherModifiers)
		{
			return Pass;
		}
		// the target app gets the queued clip instead of this Ctrl+V; holding V does not run through the queue
		return down && wasDown == false ? PasteNext : Swallow;
	}
};
