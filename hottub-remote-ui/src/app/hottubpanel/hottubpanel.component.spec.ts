import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { HottubpanelComponent } from './hottubpanel.component';

describe('HottubpanelComponent', () => {
  let component: HottubpanelComponent;
  let fixture: ComponentFixture<HottubpanelComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ HottubpanelComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(HottubpanelComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
