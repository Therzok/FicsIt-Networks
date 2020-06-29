﻿#pragma once

#include "CoreMinimal.h"
#include "Interface.h"
#include "FINGraphicsProcessor.generated.h"

UINTERFACE(Blueprintable)
class FICSITNETWORKS_API UFINGraphicsProcessor : public UInterface {
	GENERATED_BODY()
};

class FICSITNETWORKS_API IFINGraphicsProcessor {
	GENERATED_IINTERFACE_BODY()
	
public:
	/**
	 * Sets the current screen.
	 * Might cause widget recreation of screen as well as GPU reset.
	 * If nullptr is given, will just unbind the old bound screen.
	 *
	 * Server Only
	 * 
	 * @param[in]	screen	the new screen this GPU should get bound to.
	 */
	UFUNCTION()
	virtual void BindScreen(UObject* screen) = 0;

	/**
	 * Returns the currently bound screen.
	 * Nullptr if no screen is boung.
	 *
	 * @return	the currently bound screen
	 */
	UFUNCTION()
	virtual UObject* GetScreen() const = 0;

	/**
	* This gets called by the bound screen if it wants to get a new widget.
	* F.e. when the widget comes into view of the player.
	* GPU doesn't need to respond to the request.
	* If the GPU responds, should call SetWidget with the new widget onto the bound screen.
	*
	* Client Only
	*/
	UFUNCTION()
    virtual void RequestNewWidget() = 0;

	/**
	* This gets called by the bound screen if it is okay with destruction of the widget.
	* F.e. when the widget goes out of view.
	* GPU doesn't need to respond to the request.
	*
	* Client Only
	*/
	UFUNCTION()
    virtual void DropWidget() = 0;
};