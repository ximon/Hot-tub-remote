import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'hottubpanel',
  templateUrl: './hottubpanel.component.html',
  styleUrls: ['./hottubpanel.component.scss']
})
export class HottubpanelComponent implements OnInit {

  constructor() { }

  ngOnInit(): void {
  }

  changeState(state: number){
    console.log(state);
  
  }

  changeTargetTemperature(targetTemperature: number) {
    console.log(targetTemperature);
  }


}
