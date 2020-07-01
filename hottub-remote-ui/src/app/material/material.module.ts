import { NgModule } from '@angular/core';

import { MatButtonModule} from '@angular/material/button';
import { MatButtonToggleModule} from '@angular/material/button-toggle';
import { MatSliderModule } from '@angular/material/slider';
import { MatCardModule } from '@angular/material/card';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';

@NgModule({
  imports: [
    MatButtonModule, 
    MatButtonToggleModule,
    MatSliderModule,
    MatCardModule,
    MatSlideToggleModule
  ],
  exports: [
    MatButtonModule, 
    MatButtonToggleModule,
    MatSliderModule,
    MatCardModule,
    MatSlideToggleModule
  ]
})
export class MaterialModule { }
