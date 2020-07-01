import { Component, OnInit } from '@angular/core';

import { MaterialModule } from '../material/material.module';

@Component({
  selector: 'hottubpanel',
  templateUrl: './hottubpanel.component.html',
  styleUrls: ['./hottubpanel.component.scss']
})
export class HottubpanelComponent implements OnInit {

  currentTemperature: number = 0;
  targetTemperature: number = 0;

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
