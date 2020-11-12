@startuml
scale 0.75

header
model Plugin
import local example.h
private var canGetData : bool
public var timeout : bool = false
endheader

[*] -> Wait
Wait -> Wait : every 30 s / ${timeout} = true; ${canGetData} = false
Wait -down-> Run : Start
state Run {
    [*] -> CheckData
    CheckData : entry / raise Checked
    CheckData -> AddData : Checked
    state AddData {
        [*] -> Ask
        Ask : entry / raise More
        Ask : exit / raise Whatever
        Ask -> Wait : Abort / ${canGetData} = false
    }
    AddData -down-> Write : More
    AddData : entry / ${canGetData} = true
    AddData : exit / ${canGetData} = false
    Write -> CheckData : after 1 s
}
Run : exit / raise Stopped

@enduml
