# Introduction #

The 'header/footer' keyword is used to specify all model variables. This
enables the full deep C integration of the state machine. All in/out events
needs to be defined in the header or footer. This allows the generator to
create warnings if the event on a transition does not have a valid prototype.

# Syntax #

The code generator handles default PlantUML syntax, with extra features. The
extra features are added by using the *header*/*footer* keywords according to
the syntax below.

`model MODEL`

Sets the model name to MODEL. If not specified, the generator will use the
filename (without extension) for the model name.

`import relative/global X`

Declares a header X to be imported. The generated code will use the exact path
written here if the relative keyword is specified. If the global keyword is
specified, the generated code will become #include <X>. Examples:
import global myStructType.h
import relative ../mySecretStuff.h

`private/public var X : T [= Y]`

Declares a private or public variable named X of type T. The type T needs to
be accessible for the state machine, so if it is not included in stdint.h,
stdbool.h or string.h, make sure to import the required header. It is optional
to specifify the initial value of the variable, and if left blank the state
machine will memset the variable to 0 on init. Examples:
private var myStruct : MyStruct_t
public var status : int

`in/out event X [: T]`

Declares a ingoing/outgoing event named X. The type T is optional, and can be
used to define that a parameter is required to be sent when raised. The value
of the event can be accessed using the valueof(X) call in a entry/exit/oncycle
action for instance.
